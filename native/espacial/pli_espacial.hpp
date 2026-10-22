#pragma once
// PLI Espacial — fase 1: paneo VBAP 2D sobre las bocinas reales del domo.
// La posición de cada bocina sale de pruebas-domo/domo.json (la misma ficha que usa el kit de
// pruebas). Un objeto se pone en un ángulo; el renderer lo reparte entre las dos bocinas que lo
// rodean (Pulkki 1997, «Vector Base Amplitude Panning») con potencia constante: g1² + g2² = 1.
// Convención de domo.json: grados, 0 = centro, positivo hacia la derecha (horario visto desde arriba).
// El LFE (azimut null) nunca recibe señal del paneo: los graves los decide la sala (EQUIPOS-SALA.md).
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pli {

constexpr double Pi = 3.14159265358979323846;
// Orden de canales de Windows/WAVE para 7.1 (máscara 0x63F). Es el orden de salida del renderer.
constexpr int NumCanales = 8;
inline const char* IdCanal[NumCanales] = { "FL", "FR", "FC", "LFE", "BL", "BR", "SL", "SR" };

inline int indiceCanal(const std::string& id) {
    for (int i = 0; i < NumCanales; ++i) if (id == IdCanal[i]) return i;
    if (id == "C") return 2;
    throw std::runtime_error("Canal desconocido en la ficha: " + id);
}

// ---------- JSON mínimo (solo lo necesario para leer la ficha) ----------
struct Json {
    enum Tipo { Nulo, Bool, Numero, Texto, Lista, Objeto } tipo = Nulo;
    bool b = false; double n = 0; std::string s;
    std::vector<Json> lista; std::map<std::string, Json> obj;
    const Json& operator[](const std::string& k) const {
        static const Json nulo; auto it = obj.find(k); return it == obj.end() ? nulo : it->second;
    }
};

class LectorJson {
public:
    explicit LectorJson(const std::string& t) : t_(t) {}
    Json leer() { Json v = valor(); espacios(); if (p_ != t_.size()) falla("texto sobrante"); return v; }
private:
    const std::string& t_; size_t p_ = 0;
    [[noreturn]] void falla(const char* m) { throw std::runtime_error(std::string("JSON inválido (") + m + ") en la posición " + std::to_string(p_)); }
    void espacios() { while (p_ < t_.size() && std::isspace(static_cast<unsigned char>(t_[p_]))) ++p_; }
    bool toma(char c) { espacios(); if (p_ < t_.size() && t_[p_] == c) { ++p_; return true; } return false; }
    void exige(char c) { if (!toma(c)) falla("se esperaba otro símbolo"); }
    bool palabra(const char* w) { size_t n = std::char_traits<char>::length(w); if (t_.compare(p_, n, w) == 0) { p_ += n; return true; } return false; }
    std::string texto() {
        exige('"'); std::string s;
        while (p_ < t_.size() && t_[p_] != '"') {
            char c = t_[p_++];
            if (c == '\\') {
                if (p_ >= t_.size()) falla("escape incompleto");
                char e = t_[p_++];
                switch (e) {
                    case 'n': s += '\n'; break; case 't': s += '\t'; break; case 'r': s += '\r'; break;
                    case 'b': s += '\b'; break; case 'f': s += '\f'; break;
                    case 'u': if (p_ + 4 > t_.size()) falla("escape \\u incompleto"); p_ += 4; s += '?'; break;
                    default: s += e;
                }
            } else s += c;
        }
        exige('"'); return s;
    }
    Json valor() {
        espacios(); Json v;
        if (p_ >= t_.size()) falla("fin inesperado");
        char c = t_[p_];
        if (c == '{') {
            ++p_; v.tipo = Json::Objeto;
            if (toma('}')) return v;
            do { espacios(); std::string k = texto(); exige(':'); v.obj[k] = valor(); } while (toma(','));
            exige('}');
        } else if (c == '[') {
            ++p_; v.tipo = Json::Lista;
            if (toma(']')) return v;
            do v.lista.push_back(valor()); while (toma(','));
            exige(']');
        } else if (c == '"') { v.tipo = Json::Texto; v.s = texto(); }
        else if (palabra("null")) v.tipo = Json::Nulo;
        else if (palabra("true")) { v.tipo = Json::Bool; v.b = true; }
        else if (palabra("false")) { v.tipo = Json::Bool; v.b = false; }
        else {
            const char* ini = t_.c_str() + p_; char* fin = nullptr;
            v.n = std::strtod(ini, &fin);
            if (fin == ini) falla("valor no reconocido");
            v.tipo = Json::Numero; p_ += size_t(fin - ini);
        }
        return v;
    }
};

// ---------- Disposición de bocinas y paneo ----------
struct Bocina { std::string id; int canal; double azimut; };  // azimut normalizado a [0, 360)

inline double normaliza(double deg) { double a = std::fmod(deg, 360.); return a < 0 ? a + 360. : a; }

class Disposicion {
public:
    // Bocinas con azimut; las que no lo tienen (LFE) se ignoran para el paneo.
    explicit Disposicion(std::vector<Bocina> b) : b_(std::move(b)) {
        if (b_.size() < 3) throw std::runtime_error("PLI Espacial necesita al menos 3 bocinas alrededor para un anillo.");
        for (auto& x : b_) x.azimut = normaliza(x.azimut);
        std::sort(b_.begin(), b_.end(), [](const Bocina& a, const Bocina& c) { return a.azimut < c.azimut; });
        for (size_t k = 0; k < b_.size(); ++k) {
            const Bocina& a = b_[k]; const Bocina& c = b_[(k + 1) % b_.size()];
            double arco = normaliza(c.azimut - a.azimut);
            if (arco == 0) throw std::runtime_error("Dos bocinas en el mismo ángulo: " + a.id + " y " + c.id);
            if (arco >= 180) throw std::runtime_error("Hueco de " + std::to_string(int(arco)) + "° entre " + a.id + " y " + c.id + ": el paneo por pares no puede cubrirlo.");
        }
    }

    // Lee pruebas-domo/domo.json.
    static Disposicion desdeFicha(const std::string& ruta) {  // ruta en UTF-8
        std::ifstream f(std::filesystem::u8path(ruta), std::ios::binary);
        if (!f) throw std::runtime_error("No se pudo abrir la ficha " + ruta);
        std::stringstream ss; ss << f.rdbuf();
        std::string t = ss.str();
        if (t.size() >= 3 && (unsigned char)t[0] == 0xEF && (unsigned char)t[1] == 0xBB && (unsigned char)t[2] == 0xBF) t.erase(0, 3);
        Json j = LectorJson(t).leer();
        const Json& canales = j["canales"];
        if (canales.tipo != Json::Lista) throw std::runtime_error("La ficha no tiene la lista «canales».");
        std::vector<Bocina> b;
        for (const Json& c : canales.lista) {
            if (c["id"].tipo != Json::Texto) throw std::runtime_error("Un canal de la ficha no tiene «id».");
            if (c["azimut"].tipo == Json::Nulo) continue;  // LFE
            if (c["azimut"].tipo != Json::Numero) throw std::runtime_error("Azimut inválido en " + c["id"].s);
            b.push_back({ c["id"].s, indiceCanal(c["id"].s), c["azimut"].n });
        }
        return Disposicion(std::move(b));
    }

    // El anillo fijo que usaba capas71 antes de la ficha (C 0, FR 30, SR 90, BR 150, BL 210, SL 270, FL 330).
    static Disposicion anilloClasico() {
        return Disposicion({ { "FC", 2, 0 }, { "FR", 1, 30 }, { "SR", 7, 90 }, { "BR", 5, 150 },
                             { "BL", 4, 210 }, { "SL", 6, 270 }, { "FL", 0, 330 } });
    }

    const std::vector<Bocina>& bocinas() const { return b_; }

    // Ganancias VBAP 2D para un objeto en «grados». Índice = canal de salida (orden IdCanal).
    std::array<float, NumCanales> ganancias(double grados) const {
        std::array<float, NumCanales> g{};
        double th = normaliza(grados);
        size_t k = b_.size() - 1;  // por defecto, el par que cruza 360°
        for (size_t i = 0; i + 1 < b_.size(); ++i) if (th >= b_[i].azimut && th < b_[i + 1].azimut) { k = i; break; }
        const Bocina& a = b_[k]; const Bocina& c = b_[(k + 1) % b_.size()];
        auto vec = [](double deg) { double r = deg * Pi / 180; return std::array<double, 2>{ std::sin(r), std::cos(r) }; };
        auto la = vec(a.azimut), lc = vec(c.azimut), p = vec(th);
        double det = la[0] * lc[1] - la[1] * lc[0];
        double ga = (p[0] * lc[1] - p[1] * lc[0]) / det;
        double gc = (la[0] * p[1] - la[1] * p[0]) / det;
        ga = std::max(0., ga); gc = std::max(0., gc);
        double n = std::sqrt(ga * ga + gc * gc);
        g[a.canal] += float(ga / n);
        g[c.canal] += float(gc / n);
        return g;
    }

    // Suma la señal x en el bloque de 8 canales «o».
    void panea(std::array<float, NumCanales>& o, float x, double grados) const {
        auto g = ganancias(grados);
        for (int i = 0; i < NumCanales; ++i) o[i] += x * g[i];
    }

private:
    std::vector<Bocina> b_;
};

}  // namespace pli
