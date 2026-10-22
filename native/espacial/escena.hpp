#pragma once
// PLI Espacial — fase 2: escenas. Una escena es un JSON con objetos de audio (WAV mono o estéreo),
// cada uno con su movimiento en el tiempo, que se renderiza a 7.1 sobre las bocinas de la ficha.
// Formato completo y ejemplos: native/espacial/ESCENAS.md. Todos los tiempos son de la escena (s).
#include "pli_espacial.hpp"
#include "../upmix/audio71.hpp"
#include <filesystem>
#include <set>
#include <utility>

namespace pli {

// Puntos (t, x) con interpolación lineal. Antes del primero y después del último se queda fijo.
// Dos puntos con el mismo t hacen un escalón (vale el segundo).
struct Curva {
    std::vector<std::pair<double, double>> p;
    double en(double t) const {
        if (p.empty()) return 1;
        if (t < p.front().first) return p.front().second;
        if (t >= p.back().first) return p.back().second;
        size_t i = 1; while (p[i].first <= t) ++i;
        const auto& a = p[i - 1]; const auto& b = p[i];
        return a.second + (b.second - a.second) * (t - a.first) / (b.first - a.first);
    }
};

struct Movimiento {
    enum Tipo { Puntos, Giro, Vaiven } tipo = Puntos;
    Curva puntos;                                  // Puntos: azimut (grados) en el tiempo
    double inicio = 0, rpm = 0;                    // Giro: ángulo inicial y vueltas por minuto (+ horario)
    double centro = 0, amplitud = 0, periodo = 1;  // Vaivén: centro ± amplitud, periodo en s
    Curva velocidad;                               // Giro: multiplica las rpm. Vaivén: multiplica la amplitud (tope 1)
};

// Avanza un movimiento muestra por muestra (el giro se integra: la velocidad puede cambiar).
class Recorrido {
public:
    Recorrido(const Movimiento& m, unsigned rate) : m_(m), rate_(rate), theta_(m.inicio) {}
    double siguiente() {
        double t = double(i_++) / rate_;
        switch (m_.tipo) {
            case Movimiento::Giro: theta_ += m_.rpm * m_.velocidad.en(t) * 360. / 60. / rate_; return theta_;
            case Movimiento::Vaiven: return m_.centro + m_.amplitud * std::sin(2 * Pi * t / m_.periodo) * std::min(1., m_.velocidad.en(t));
            default: return m_.puntos.en(t);
        }
    }
private:
    const Movimiento& m_; unsigned rate_; double theta_; size_t i_ = 0;
};

struct Objeto {
    std::string nombre, archivo;
    bool mono = false;       // true: se suma L+R y se panea como un punto
    double ganancia = 1;     // lineal (en el JSON va en dB: "nivel_db")
    double ancho = 0;        // estéreo: L en azimut − ancho, R en azimut + ancho
    double inicio = 0;       // segundo de la escena en que empieza a sonar
    bool lfe = false;        // true: va directo al LFE (mono, sin paneo ni filtro); p. ej. el LFE de un ADM
    Movimiento mov;
    Stereo audio;
};

struct Lfe { bool activo = false; std::vector<std::string> fuentes; double corte = 80, ganancia = .5; };

struct Escena {
    std::string nombre, ficha;  // ficha vacía = anillo clásico
    std::vector<Objeto> objetos;
    Lfe lfe;
};

// ---------- Lectura ----------
inline Curva leeCurva(const Json& j, const std::string& donde) {
    Curva c;
    if (j.tipo != Json::Lista) throw std::runtime_error(donde + ": se espera una lista de [tiempo, valor].");
    for (const Json& p : j.lista) {
        if (p.tipo != Json::Lista || p.lista.size() != 2 || p.lista[0].tipo != Json::Numero || p.lista[1].tipo != Json::Numero)
            throw std::runtime_error(donde + ": cada punto debe ser [tiempo, valor].");
        if (!c.p.empty() && p.lista[0].n < c.p.back().first) throw std::runtime_error(donde + ": los tiempos deben ir en orden.");
        c.p.push_back({ p.lista[0].n, p.lista[1].n });
    }
    if (c.p.empty()) throw std::runtime_error(donde + ": la lista está vacía.");
    return c;
}

inline double numero(const Json& j, const std::string& k, double defecto, const std::string& donde) {
    const Json& v = j[k];
    if (v.tipo == Json::Nulo) return defecto;
    if (v.tipo != Json::Numero) throw std::runtime_error(donde + ": «" + k + "» debe ser un número.");
    return v.n;
}

inline Escena interpretaEscena(const Json& j, const std::string& carpeta) {
    namespace fs = std::filesystem;
    if (j.tipo != Json::Objeto) throw std::runtime_error("La escena debe ser un objeto JSON.");
    Escena e;
    e.nombre = j["nombre"].tipo == Json::Texto ? j["nombre"].s : "escena";
    auto resuelve = [&](const std::string& r) { fs::path p = fs::u8path(r); return (p.is_absolute() ? p : fs::u8path(carpeta) / p).lexically_normal().u8string(); };
    if (j["ficha"].tipo == Json::Texto) e.ficha = resuelve(j["ficha"].s);
    else if (j["ficha"].tipo != Json::Nulo) throw std::runtime_error("«ficha» debe ser la ruta a domo.json o null.");
    Curva velGlobal; bool hayVelGlobal = j["velocidad"].tipo != Json::Nulo;
    if (hayVelGlobal) velGlobal = leeCurva(j["velocidad"], "velocidad de la escena");

    if (j["objetos"].tipo != Json::Lista || j["objetos"].lista.empty()) throw std::runtime_error("La escena no tiene «objetos».");
    std::set<std::string> nombres;
    for (const Json& o : j["objetos"].lista) {
        Objeto ob;
        if (o["nombre"].tipo != Json::Texto || o["nombre"].s.empty()) throw std::runtime_error("Un objeto no tiene «nombre».");
        ob.nombre = o["nombre"].s;
        const std::string d = "objeto «" + ob.nombre + "»";
        if (!nombres.insert(ob.nombre).second) throw std::runtime_error("Nombre repetido: " + ob.nombre);
        if (o["archivo"].tipo != Json::Texto) throw std::runtime_error(d + ": falta «archivo».");
        ob.archivo = resuelve(o["archivo"].s);
        ob.mono = o["mono"].tipo == Json::Bool && o["mono"].b;
        ob.ganancia = std::pow(10., numero(o, "nivel_db", 0, d) / 20);
        ob.ancho = numero(o, "ancho", 0, d);
        ob.inicio = numero(o, "inicio_s", 0, d);
        if (ob.ancho < 0 || ob.ancho > 180) throw std::runtime_error(d + ": «ancho» va de 0 a 180 grados.");
        if (ob.inicio < 0) throw std::runtime_error(d + ": «inicio_s» no puede ser negativo.");
        ob.lfe = o["lfe"].tipo == Json::Bool && o["lfe"].b;
        const Json& m = o["movimiento"];
        if (ob.lfe) ob.mov.puntos.p = { { 0, 0 } };  // no se panea
        else if (m.tipo == Json::Nulo) {
            if (o["azimut"].tipo != Json::Numero) throw std::runtime_error(d + ": necesita «azimut» (fijo) o «movimiento».");
            ob.mov.puntos.p = { { 0, o["azimut"].n } };
        } else {
            std::string tipo = m["tipo"].tipo == Json::Texto ? m["tipo"].s : "";
            if (tipo == "puntos") { ob.mov.tipo = Movimiento::Puntos; ob.mov.puntos = leeCurva(m["puntos"], d + " (puntos)"); }
            else if (tipo == "giro") { ob.mov.tipo = Movimiento::Giro; ob.mov.inicio = numero(m, "inicio", 0, d); ob.mov.rpm = numero(m, "rpm", 0, d); }
            else if (tipo == "vaiven") {
                ob.mov.tipo = Movimiento::Vaiven; ob.mov.centro = numero(m, "centro", 0, d);
                ob.mov.amplitud = numero(m, "amplitud", 0, d); ob.mov.periodo = numero(m, "periodo_s", 0, d);
                if (ob.mov.periodo <= 0) throw std::runtime_error(d + ": «periodo_s» debe ser mayor que 0.");
            } else throw std::runtime_error(d + ": «tipo» de movimiento debe ser puntos, giro o vaiven.");
            if (m["velocidad"].tipo != Json::Nulo) ob.mov.velocidad = leeCurva(m["velocidad"], d + " (velocidad)");
            else if (hayVelGlobal) ob.mov.velocidad = velGlobal;
        }
        e.objetos.push_back(std::move(ob));
    }

    const Json& l = j["lfe"];
    if (l.tipo != Json::Nulo) {
        e.lfe.activo = true;
        if (l["fuentes"].tipo != Json::Lista || l["fuentes"].lista.empty()) throw std::runtime_error("lfe: falta la lista «fuentes».");
        for (const Json& f : l["fuentes"].lista) {
            if (f.tipo != Json::Texto || !nombres.count(f.s)) throw std::runtime_error("lfe: la fuente «" + f.s + "» no es un objeto de la escena.");
            e.lfe.fuentes.push_back(f.s);
        }
        e.lfe.corte = numero(l, "corte_hz", 80, "lfe");
        e.lfe.ganancia = numero(l, "ganancia", .5, "lfe");
        if (e.lfe.corte < 20 || e.lfe.corte > 200) throw std::runtime_error("lfe: «corte_hz» va de 20 a 200.");
    }
    return e;
}

inline Escena leeEscena(const std::string& ruta) {
    std::ifstream f(std::filesystem::u8path(ruta), std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo abrir la escena " + ruta);
    std::stringstream ss; ss << f.rdbuf(); std::string t = ss.str();
    if (t.size() >= 3 && (unsigned char)t[0] == 0xEF && (unsigned char)t[1] == 0xBB && (unsigned char)t[2] == 0xBF) t.erase(0, 3);
    Escena e = interpretaEscena(LectorJson(t).leer(), std::filesystem::u8path(ruta).parent_path().u8string());
    for (auto& o : e.objetos) o.audio = readWav(o.archivo);
    return e;
}

// ---------- Render ----------
inline unsigned frecuenciaComun(const Escena& e) {
    unsigned rate = 0;
    for (const auto& o : e.objetos) {
        if (!rate) rate = o.audio.rate;
        else if (o.audio.rate != rate) throw std::runtime_error("«" + o.nombre + "» está a " + std::to_string(o.audio.rate) + " Hz y la escena a " + std::to_string(rate) + " Hz: todos los WAV deben tener la misma frecuencia.");
    }
    if (!rate) throw std::runtime_error("Frecuencia de muestreo desconocida.");
    return rate;
}

inline Multi renderiza(const Escena& e, const Disposicion& d) {
    const unsigned rate = frecuenciaComun(e);
    std::vector<size_t> off; size_t len = 0;
    for (const auto& o : e.objetos) { off.push_back(size_t(std::llround(o.inicio * rate))); len = std::max(len, off.back() + o.audio.l.size()); }
    std::vector<Recorrido> rec; rec.reserve(e.objetos.size());
    for (const auto& o : e.objetos) rec.emplace_back(o.mov, rate);
    std::vector<bool> aLfe(e.objetos.size(), false);
    for (size_t k = 0; k < e.objetos.size(); ++k)
        for (auto& f : e.lfe.fuentes) if (f == e.objetos[k].nombre) aLfe[k] = true;
    Biquad lp1 = Biquad::make(false, e.lfe.corte, rate), lp2 = Biquad::make(false, e.lfe.corte, rate);

    Multi out(len);
    for (size_t i = 0; i < len; ++i) {
        auto& o = out[i]; float graves = 0;
        for (size_t k = 0; k < e.objetos.size(); ++k) {
            const Objeto& ob = e.objetos[k];
            double az = rec[k].siguiente();  // avanza siempre: el movimiento sigue el reloj de la escena
            if (i < off[k] || i - off[k] >= ob.audio.l.size()) continue;
            float l = float(ob.ganancia) * ob.audio.l[i - off[k]], r = float(ob.ganancia) * ob.audio.r[i - off[k]];
            if (ob.lfe) o[LFE] += .5f * (l + r);
            else if (ob.mono) d.panea(o, .5f * (l + r), az);
            else { d.panea(o, l, az - ob.ancho); d.panea(o, r, az + ob.ancho); }
            if (aLfe[k]) graves += .5f * (l + r);
        }
        if (e.lfe.activo) o[LFE] += float(e.lfe.ganancia) * lp2(lp1(graves));
    }
    return out;
}

}  // namespace pli
