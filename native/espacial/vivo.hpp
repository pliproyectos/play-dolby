#pragma once
// PLI Espacial — fase 3: objetos en vivo dentro de Play Dolby.
// Cada objeto toma un canal de entrada (USB de la consola) y lo panea sobre las bocinas de la ficha.
// Se suma a las salidas directas del puente, en el mismo bloque de captura: no agrega latencia.
//
// Hilos: la interfaz escribe los parámetros (atómicos); el callback de captura los lee una vez por
// bloque. El callback no reserva memoria ni toma candados. La Disposicion solo se cambia con el
// puente detenido (prepara()).
// Sin chasquidos: las ganancias van en rampa lineal dentro de cada bloque; al cambiar de entrada,
// la anterior baja a cero en un bloque y la nueva sube en el siguiente.
#include "pli_espacial.hpp"
#include <atomic>

namespace pli {

constexpr int MaxObjetos = 8;
using Cuadro = std::array<float, NumCanales>;

enum class Modo : int { Fijo = 0, Giro = 1, Vaiven = 2 };

struct ObjetoVivo {
    std::atomic<int> entrada{ -1 };       // canal de entrada (0 = USB 1); -1 = apagado. Estéreo: L = entrada, R = entrada + 1
    std::atomic<bool> estereo{ false };   // true: L en azimut − ancho, R en azimut + ancho
    std::atomic<float> ancho{ 30 };       // grados (solo estéreo)
    std::atomic<int> modo{ int(Modo::Fijo) };
    std::atomic<float> azimut{ 0 };       // fijo: posición; giro: desfase; vaivén: centro
    std::atomic<float> rpm{ 0 };          // giro: vueltas por minuto (+ horario)
    std::atomic<float> amplitud{ 30 };    // vaivén: grados a cada lado
    std::atomic<float> periodo{ 8 };      // vaivén: segundos por ciclo
    std::atomic<float> nivel{ 1 };        // ganancia lineal
    std::atomic<float> posicion{ 0 };     // la escribe el audio: dónde está ahora (para dibujar)
};

class Espacio {
public:
    std::array<ObjetoVivo, MaxObjetos> obj;
    std::atomic<bool> activo{ false };
    std::atomic<float> velocidad{ 1 };  // velocidad maestra: multiplica giros y vaivenes (0 = congelados)

    // Solo con el puente detenido.
    void prepara(const Disposicion* d, double rate) {
        d_ = d; rate_ = rate > 0 ? rate : 48000;
        for (int k = 0; k < MaxObjetos; ++k) { fase_[k] = 0; g_[k] = {}; gR_[k] = {}; actual_[k] = -1; estereoActual_[k] = false; }
    }
    const Disposicion* disposicion() const { return d_; }

    // Suma los objetos del bloque «in» (frames × count canales intercalados) en «out».
    void suma(const float* in, unsigned frames, unsigned count, Cuadro* out) {
        if (!d_ || !in || !frames) return;
        const double dt = double(frames) / rate_ * std::max(0.f, velocidad.load(std::memory_order_relaxed));
        for (int k = 0; k < MaxObjetos; ++k) {
            ObjetoVivo& o = obj[k];
            const int pedida = o.entrada.load(std::memory_order_relaxed);
            // Movimiento: avanza aunque el objeto esté callado, para que siga el reloj.
            const Modo modo = Modo(o.modo.load(std::memory_order_relaxed));
            const double base = o.azimut.load(std::memory_order_relaxed);
            double az = base;
            if (modo == Modo::Giro) {
                fase_[k] = std::fmod(fase_[k] + o.rpm.load(std::memory_order_relaxed) * 6. * dt, 360.);
                az = base + fase_[k];
            } else if (modo == Modo::Vaiven) {
                const double per = std::max(.1f, o.periodo.load(std::memory_order_relaxed));
                fase_[k] = std::fmod(fase_[k] + dt, per);
                az = base + o.amplitud.load(std::memory_order_relaxed) * std::sin(2 * Pi * fase_[k] / per);
            } else fase_[k] = 0;
            o.posicion.store(float(normaliza(az)), std::memory_order_relaxed);

            // Destino de ganancias. Si cambió la entrada (o pasó de mono a estéreo), primero se apaga la anterior.
            const bool est = o.estereo.load(std::memory_order_relaxed);
            const bool cambio = pedida != actual_[k] || est != estereoActual_[k];
            const int fuente = cambio ? actual_[k] : pedida;
            const bool fuenteEst = cambio ? estereoActual_[k] : est;
            Cuadro destino{}, destinoR{};
            if (!cambio && fuente >= 0) {
                const float nivel = std::max(0.f, o.nivel.load(std::memory_order_relaxed));
                const double ancho = est ? o.ancho.load(std::memory_order_relaxed) : 0.;
                destino = d_->ganancias(az - ancho);
                if (est) destinoR = d_->ganancias(az + ancho);
                for (int c = 0; c < NumCanales; ++c) { destino[c] *= nivel; destinoR[c] *= nivel; }
            }
            Cuadro& g = g_[k]; Cuadro& gR = gR_[k];
            const float paso = 1.f / float(frames);
            auto rampa = [&](int canal, Cuadro& actual, const Cuadro& meta) {
                if (canal < 0 || unsigned(canal) >= count) return;
                for (unsigned i = 0; i < frames; ++i) {
                    float x = in[size_t(i) * count + unsigned(canal)];
                    if (!std::isfinite(x)) continue;
                    const float t = float(i + 1) * paso;
                    for (int c = 0; c < NumCanales; ++c) out[i][c] += x * (actual[c] + (meta[c] - actual[c]) * t);
                }
            };
            if (fuente >= 0) { rampa(fuente, g, destino); if (fuenteEst) rampa(fuente + 1, gR, destinoR); }
            g = destino; gR = destinoR;
            if (cambio) { actual_[k] = pedida; estereoActual_[k] = est; }  // la nueva entrada sube desde 0 en el siguiente bloque
        }
    }

private:
    const Disposicion* d_ = nullptr;
    double rate_ = 48000;
    std::array<double, MaxObjetos> fase_{};
    std::array<Cuadro, MaxObjetos> g_{}, gR_{};
    std::array<int, MaxObjetos> actual_{ -1, -1, -1, -1, -1, -1, -1, -1 };
    std::array<bool, MaxObjetos> estereoActual_{};
};

}  // namespace pli
