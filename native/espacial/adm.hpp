#pragma once
// PLI Espacial — fase 4: ADM (ITU-R BS.2076) en archivos BW64/WAV, sin librerías externas.
// Un archivo ADM es un WAV con N pistas mono + un bloque «chna» (pista → identificadores) + un
// bloque «axml» (XML con la posición de cada objeto en el tiempo).
//
// Exportar: cada objeto mono es una pista «Objects»; un objeto estéreo son dos (L en azimut − ancho,
// R en azimut + ancho); lo que va al LFE es una pista «DirectSpeakers» LFE. Las trayectorias se
// muestrean cada 0,1 s (bloques con interpolación lineal); un objeto fijo es un solo bloque.
// Importar: «Objects» → objeto con trayectoria por puntos; «DirectSpeakers» → objeto fijo (o LFE).
// HOA, Matrix y Binaural no se soportan: se avisan y se omiten.
//
// Convenciones: en ADM el azimut positivo va a la IZQUIERDA; en PLI Espacial, a la derecha. Se
// invierte el signo al pasar de uno a otro. La elevación se ignora: el domo no tiene bocinas de
// altura (se avisa cuántos objetos la traían).
#include "escena.hpp"
#include <cinttypes>
#include <iomanip>

namespace pli::adm {

// ---------- XML mínimo ----------
struct Nodo {
    std::string nombre, texto;  // nombre sin prefijo de espacio de nombres
    std::map<std::string, std::string> attr;
    std::vector<Nodo> hijos;
    const Nodo* hijo(const std::string& n) const { for (auto& h : hijos) if (h.nombre == n) return &h; return nullptr; }
    std::string a(const std::string& k) const { auto it = attr.find(k); return it == attr.end() ? "" : it->second; }
};

class LectorXml {
public:
    explicit LectorXml(const std::string& t) : t_(t) {}
    Nodo leer() {
        Nodo raiz; raiz.nombre = "#documento";
        std::vector<Nodo*> pila{ &raiz };
        while (p_ < t_.size()) {
            if (t_[p_] != '<') { size_t f = t_.find('<', p_); if (f == std::string::npos) f = t_.size(); pila.back()->texto += decodifica(t_.substr(p_, f - p_)); p_ = f; continue; }
            if (t_.compare(p_, 4, "<!--") == 0) { salta("-->"); continue; }
            if (t_.compare(p_, 2, "<?") == 0) { salta("?>"); continue; }
            if (t_.compare(p_, 9, "<![CDATA[") == 0) { size_t f = t_.find("]]>", p_); if (f == std::string::npos) falla("CDATA sin cerrar"); pila.back()->texto += t_.substr(p_ + 9, f - p_ - 9); p_ = f + 3; continue; }
            if (t_.compare(p_, 2, "<!") == 0) { salta(">"); continue; }
            if (t_.compare(p_, 2, "</") == 0) {
                size_t f = t_.find('>', p_); if (f == std::string::npos) falla("etiqueta sin cerrar");
                if (pila.size() < 2) falla("cierre sin apertura");
                if (local(t_.substr(p_ + 2, f - p_ - 2)) != pila.back()->nombre) falla(("cierre de «" + pila.back()->nombre + "» no coincide").c_str());
                pila.pop_back(); p_ = f + 1; continue;
            }
            ++p_; Nodo n; n.nombre = local(nombre());
            for (;;) {
                espacios();
                if (p_ >= t_.size()) falla("fin inesperado");
                if (t_[p_] == '/') { if (p_ + 1 >= t_.size() || t_[p_ + 1] != '>') falla("se esperaba />"); p_ += 2; pila.back()->hijos.push_back(std::move(n)); break; }
                if (t_[p_] == '>') { ++p_; pila.back()->hijos.push_back(std::move(n)); pila.push_back(&pila.back()->hijos.back()); break; }
                std::string k = local(nombre()); espacios();
                if (p_ >= t_.size() || t_[p_] != '=') falla("atributo sin =");
                ++p_; espacios();
                if (p_ >= t_.size() || (t_[p_] != '"' && t_[p_] != '\'')) falla("atributo sin comillas");
                char q = t_[p_++]; size_t f = t_.find(q, p_); if (f == std::string::npos) falla("atributo sin cerrar");
                n.attr[k] = decodifica(t_.substr(p_, f - p_)); p_ = f + 1;
            }
        }
        if (pila.size() != 1) falla("elementos sin cerrar");
        return raiz;
    }
private:
    const std::string& t_; size_t p_ = 0;
    [[noreturn]] void falla(const char* m) { throw std::runtime_error(std::string("XML ADM inválido (") + m + ") cerca del byte " + std::to_string(p_)); }
    void salta(const char* fin) { size_t f = t_.find(fin, p_); if (f == std::string::npos) falla("comentario o declaración sin cerrar"); p_ = f + std::strlen(fin); }
    void espacios() { while (p_ < t_.size() && std::isspace(static_cast<unsigned char>(t_[p_]))) ++p_; }
    std::string nombre() { size_t i = p_; while (p_ < t_.size() && !std::isspace(static_cast<unsigned char>(t_[p_])) && t_[p_] != '>' && t_[p_] != '/' && t_[p_] != '=') ++p_; if (i == p_) falla("nombre vacío"); return t_.substr(i, p_ - i); }
    static std::string local(const std::string& n) { size_t c = n.find(':'); return c == std::string::npos ? n : n.substr(c + 1); }
    static std::string decodifica(const std::string& s) {
        std::string r;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] != '&') { r += s[i]; continue; }
            size_t f = s.find(';', i); if (f == std::string::npos) { r += s[i]; continue; }
            std::string e = s.substr(i + 1, f - i - 1);
            if (e == "amp") r += '&'; else if (e == "lt") r += '<'; else if (e == "gt") r += '>'; else if (e == "quot") r += '"'; else if (e == "apos") r += '\'';
            else if (!e.empty() && e[0] == '#') { long c = e.size() > 1 && e[1] == 'x' ? std::strtol(e.c_str() + 2, nullptr, 16) : std::strtol(e.c_str() + 1, nullptr, 10); if (c > 0 && c < 128) r += char(c); else r += '?'; }
            else r += "&" + e + ";";
            i = f;
        }
        return r;
    }
};

inline std::string escapa(const std::string& s) {
    std::string r;
    for (char c : s) { if (c == '&') r += "&amp;"; else if (c == '<') r += "&lt;"; else if (c == '>') r += "&gt;"; else if (c == '"') r += "&quot;"; else r += c; }
    return r;
}

// ---------- Tiempo ADM: «hh:mm:ss.fffff» o por muestras «hh:mm:ss.nnnnnSrrrrr» ----------
inline std::string tiempo(double s) {
    long long cent = std::llround(s * 100000);
    char b[32]; std::snprintf(b, sizeof b, "%02lld:%02lld:%02lld.%05lld", cent / 360000000, cent / 6000000 % 60, cent / 100000 % 60, cent % 100000);
    return b;
}
inline double leeTiempo(const std::string& t) {
    if (t.empty()) return 0;
    int h = 0, m = 0; double s = 0; size_t S = t.find('S');
    if (S != std::string::npos) {
        long long seg = 0, muestras = 0, rate = 0;
        if (std::sscanf(t.c_str(), "%d:%d:%lld.%lldS%lld", &h, &m, &seg, &muestras, &rate) != 5 || rate <= 0) throw std::runtime_error("Tiempo ADM inválido: " + t);
        return h * 3600. + m * 60. + double(seg) + double(muestras) / double(rate);
    }
    if (std::sscanf(t.c_str(), "%d:%d:%lf", &h, &m, &s) != 3) throw std::runtime_error("Tiempo ADM inválido: " + t);
    return h * 3600. + m * 60. + s;
}

// ---------- Pistas ----------
enum class Tipo { Objeto, Altavoz, Lfe };
struct Pista {
    std::string nombre; Tipo tipo = Tipo::Objeto;
    std::vector<float> audio;
    Curva azimut;  // en convención PLI (positivo a la derecha); para Altavoz/LFE, un solo punto
};

struct Archivo { unsigned rate = 48000; std::vector<Pista> pistas; std::string avisos; };

// ---------- Escena → pistas (exportar) ----------
// La escena debe tener el audio cargado. Paso de muestreo de trayectorias: «paso» segundos.
inline Archivo desdeEscena(const Escena& e, double paso = .1) {
    Archivo a; a.rate = frecuenciaComun(e);
    const unsigned rate = a.rate;
    size_t len = 0; std::vector<size_t> off;
    for (auto& o : e.objetos) { off.push_back(size_t(std::llround(o.inicio * rate))); len = std::max(len, off.back() + o.audio.l.size()); }
    const size_t salto = std::max<size_t>(1, size_t(std::llround(paso * rate)));
    std::vector<float> graves(e.lfe.activo ? len : 0, 0.f);
    for (size_t k = 0; k < e.objetos.size(); ++k) {
        const Objeto& ob = e.objetos[k];
        auto senal = [&](int lado) {  // 0 L, 1 R, 2 mono; ya con nivel y desfase de inicio
            std::vector<float> s(len, 0.f);
            for (size_t i = 0; i < ob.audio.l.size(); ++i) {
                float l = float(ob.ganancia) * ob.audio.l[i], r = float(ob.ganancia) * ob.audio.r[i];
                s[off[k] + i] = lado == 0 ? l : lado == 1 ? r : .5f * (l + r);
            }
            return s;
        };
        bool aLfe = false; for (auto& f : e.lfe.fuentes) aLfe = aLfe || f == ob.nombre;
        if (aLfe) { auto m = senal(2); for (size_t i = 0; i < len; ++i) graves[i] += m[i]; }
        if (ob.lfe) { Pista p; p.nombre = ob.nombre; p.tipo = Tipo::Lfe; p.audio = senal(2); p.azimut.p = { { 0, 0 } }; a.pistas.push_back(std::move(p)); continue; }
        // Trayectoria muestreada (el giro se integra muestra a muestra, como en el render).
        Curva c; Recorrido r(ob.mov, rate);
        const bool fijo = ob.mov.tipo == Movimiento::Puntos && ob.mov.puntos.p.size() == 1;
        double anterior = 0;
        for (size_t i = 0; i < len; ++i) {
            double az = r.siguiente();
            if (fijo) { c.p = { { 0, az } }; break; }
            if (i % salto == 0 || i + 1 == len) {
                if (!c.p.empty()) az = anterior + std::remainder(az - anterior, 360.);  // sin saltos de 360
                c.p.push_back({ double(i) / rate, az }); anterior = az;
            }
        }
        if (ob.mono) { Pista p; p.nombre = ob.nombre; p.audio = senal(2); p.azimut = c; a.pistas.push_back(std::move(p)); }
        else for (int lado = 0; lado < 2; ++lado) {
            Pista p; p.nombre = ob.nombre + (lado ? " R" : " L"); p.audio = senal(lado); p.azimut = c;
            for (auto& q : p.azimut.p) q.second += lado ? ob.ancho : -ob.ancho;
            a.pistas.push_back(std::move(p));
        }
    }
    if (e.lfe.activo) {  // el LFE filtrado de la escena se exporta ya renderizado
        Biquad lp1 = Biquad::make(false, e.lfe.corte, rate), lp2 = Biquad::make(false, e.lfe.corte, rate);
        Pista p; p.nombre = "LFE"; p.tipo = Tipo::Lfe; p.azimut.p = { { 0, 0 } }; p.audio.resize(len);
        for (size_t i = 0; i < len; ++i) p.audio[i] = float(e.lfe.ganancia) * lp2(lp1(graves[i]));
        a.pistas.push_back(std::move(p));
    }
    return a;
}

// ---------- XML + chna ----------
inline std::string hex4(unsigned v) { char b[8]; std::snprintf(b, sizeof b, "%04X", v); return b; }
inline std::string hex8(unsigned v) { char b[12]; std::snprintf(b, sizeof b, "%08X", v); return b; }
inline double admAz(double ours) { double a = -std::remainder(ours, 360.); return a == -180 ? 180 : (a == 0 ? 0 : a); }

struct Metadatos { std::string xml; std::vector<uint8_t> chna; };

inline Metadatos escribeMetadatos(const Archivo& a, const std::string& titulo) {
    std::ostringstream x; x << std::fixed;
    const double dur = a.pistas.empty() ? 0 : double(a.pistas[0].audio.size()) / a.rate;
    auto ids = [](const Pista& p, unsigned n) {
        const std::string t = p.tipo == Tipo::Objeto ? "0003" : "0001";
        return std::array<std::string, 6>{ "AP_" + t + hex4(0x1000 + n), "AC_" + t + hex4(0x1000 + n), "AS_" + t + hex4(0x1000 + n),
                                           "AT_" + t + hex4(0x1000 + n) + "_01", "ATU_" + hex8(n), "AO_" + hex4(0x1000 + n) };
    };
    x << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<ebuCoreMain xmlns=\"urn:ebu:metadata-schema:ebuCore_2014\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" schema=\"EBU_CORE_20140201.xsd\" xml:lang=\"en\">\n"
      << "<coreMetadata><format><audioFormatExtended version=\"ITU-R_BS.2076-2\">\n"
      << "<audioProgramme audioProgrammeID=\"APR_1001\" audioProgrammeName=\"" << escapa(titulo) << "\" start=\"" << tiempo(0) << "\" end=\"" << tiempo(dur) << "\">\n"
      << "  <audioContentIDRef>ACO_1001</audioContentIDRef>\n</audioProgramme>\n"
      << "<audioContent audioContentID=\"ACO_1001\" audioContentName=\"" << escapa(titulo) << "\">\n";
    for (unsigned n = 1; n <= a.pistas.size(); ++n) x << "  <audioObjectIDRef>" << ids(a.pistas[n - 1], n)[5] << "</audioObjectIDRef>\n";
    x << "</audioContent>\n";
    for (unsigned n = 1; n <= a.pistas.size(); ++n) {
        const Pista& p = a.pistas[n - 1]; auto id = ids(p, n);
        const bool obj = p.tipo == Tipo::Objeto;
        const std::string tl = obj ? "0003" : "0001", td = obj ? "Objects" : "DirectSpeakers", nom = escapa(p.nombre);
        x << "<audioObject audioObjectID=\"" << id[5] << "\" audioObjectName=\"" << nom << "\" start=\"" << tiempo(0) << "\" duration=\"" << tiempo(dur) << "\">\n"
          << "  <audioPackFormatIDRef>" << id[0] << "</audioPackFormatIDRef>\n  <audioTrackUIDRef>" << id[4] << "</audioTrackUIDRef>\n</audioObject>\n"
          << "<audioPackFormat audioPackFormatID=\"" << id[0] << "\" audioPackFormatName=\"" << nom << "\" typeLabel=\"" << tl << "\" typeDefinition=\"" << td << "\">\n"
          << "  <audioChannelFormatIDRef>" << id[1] << "</audioChannelFormatIDRef>\n</audioPackFormat>\n"
          << "<audioChannelFormat audioChannelFormatID=\"" << id[1] << "\" audioChannelFormatName=\"" << nom << "\" typeLabel=\"" << tl << "\" typeDefinition=\"" << td << "\">\n";
        if (p.tipo == Tipo::Lfe) x << "  <frequency typeDefinition=\"lowPass\">120</frequency>\n";
        auto bloque = [&](unsigned b, double ini, double fin, double az) {
            x << "  <audioBlockFormat audioBlockFormatID=\"AB_" << tl << hex4(0x1000 + n) << "_" << hex8(b) << "\" rtime=\"" << tiempo(ini) << "\" duration=\"" << tiempo(fin - ini) << "\">\n";
            if (p.tipo == Tipo::Objeto) x << std::setprecision(4) << "    <position coordinate=\"azimuth\">" << admAz(az) << "</position>\n    <position coordinate=\"elevation\">0.0000</position>\n    <position coordinate=\"distance\">1.0000</position>\n";
            else x << "    <speakerLabel>" << (p.tipo == Tipo::Lfe ? "LFE1" : "M") << "</speakerLabel>\n" << std::setprecision(4)
                   << "    <position coordinate=\"azimuth\">" << (p.tipo == Tipo::Lfe ? 0.0 : admAz(p.azimut.p[0].second)) << "</position>\n    <position coordinate=\"elevation\">" << (p.tipo == Tipo::Lfe ? -30.0 : 0.0) << "</position>\n";
            x << "  </audioBlockFormat>\n";
        };
        const auto& c = p.azimut.p;
        if (c.size() <= 1) bloque(1, 0, dur, c.empty() ? 0 : c[0].second);
        else  // bloques contiguos [t(i-1), t(i)] que llegan a su punto al final (interpolación lineal)
            for (size_t i = 1; i < c.size(); ++i) bloque(unsigned(i), c[i - 1].first, i + 1 < c.size() ? c[i].first : std::max(c[i].first, dur), c[i].second);
        x << "</audioChannelFormat>\n"
          << "<audioStreamFormat audioStreamFormatID=\"" << id[2] << "\" audioStreamFormatName=\"" << nom << "\" formatLabel=\"0001\" formatDefinition=\"PCM\">\n"
          << "  <audioChannelFormatIDRef>" << id[1] << "</audioChannelFormatIDRef>\n  <audioTrackFormatIDRef>" << id[3] << "</audioTrackFormatIDRef>\n</audioStreamFormat>\n"
          << "<audioTrackFormat audioTrackFormatID=\"" << id[3] << "\" audioTrackFormatName=\"" << nom << "\" formatLabel=\"0001\" formatDefinition=\"PCM\">\n"
          << "  <audioStreamFormatIDRef>" << id[2] << "</audioStreamFormatIDRef>\n</audioTrackFormat>\n"
          << "<audioTrackUID UID=\"" << id[4] << "\" sampleRate=\"" << a.rate << "\" bitDepth=\"24\">\n"
          << "  <audioTrackFormatIDRef>" << id[3] << "</audioTrackFormatIDRef>\n  <audioPackFormatIDRef>" << id[0] << "</audioPackFormatIDRef>\n</audioTrackUID>\n";
    }
    x << "</audioFormatExtended></format></coreMetadata>\n</ebuCoreMain>\n";
    Metadatos m; m.xml = x.str();
    auto w16 = [&](unsigned v) { m.chna.push_back(uint8_t(v)); m.chna.push_back(uint8_t(v >> 8)); };
    auto ws = [&](const std::string& s, size_t n) { for (size_t i = 0; i < n; ++i) m.chna.push_back(i < s.size() ? uint8_t(s[i]) : 0); };
    w16(unsigned(a.pistas.size())); w16(unsigned(a.pistas.size()));
    for (unsigned n = 1; n <= a.pistas.size(); ++n) { auto id = ids(a.pistas[n - 1], n); w16(n); ws(id[4], 12); ws(id[3], 14); ws(id[0], 11); m.chna.push_back(0); }
    return m;
}

// ---------- XML + chna → pistas (importar) ----------
inline void reune(const Nodo& n, std::map<std::string, std::map<std::string, const Nodo*>>& idx) {
    static const char* tipos[][2] = { { "audioChannelFormat", "audioChannelFormatID" }, { "audioStreamFormat", "audioStreamFormatID" },
                                      { "audioTrackFormat", "audioTrackFormatID" }, { "audioTrackUID", "UID" }, { "audioPackFormat", "audioPackFormatID" }, { "audioObject", "audioObjectID" } };
    for (auto& t : tipos) if (n.nombre == t[0]) idx[t[0]][n.a(t[1])] = &n;
    for (auto& h : n.hijos) reune(h, idx);
}

inline double coordenada(const Nodo& b, const std::string& c, double defecto, bool* hay = nullptr) {
    for (auto& h : b.hijos) if (h.nombre == "position" && h.a("coordinate") == c) { if (hay) *hay = true; return std::strtod(h.texto.c_str(), nullptr); }
    if (hay) *hay = false;
    return defecto;
}

// Azimut PLI de un bloque (polar o cartesiano).
inline double azimutBloque(const Nodo& b, double& elev) {
    bool hayX = false, hayAz = false;
    double x = coordenada(b, "X", 0, &hayX), az = coordenada(b, "azimuth", 0, &hayAz);
    if (hayX || b.a("cartesian") == "1") { double y = coordenada(b, "Y", 1), z = coordenada(b, "Z", 0); elev = std::atan2(z, std::hypot(x, y)) * 180 / Pi; return std::atan2(x, y) * 180 / Pi; }
    elev = coordenada(b, "elevation", 0);
    return -az;
}

// Definiciones comunes (ITU-R BS.2094) que suelen no venir en el XML: solo las de 5.1 que conocemos con certeza.
inline bool definicionComun(const std::string& ac, Pista& p) {
    static const std::map<std::string, std::pair<const char*, double>> tabla = {
        { "AC_00010001", { "L", -30 } }, { "AC_00010002", { "R", 30 } }, { "AC_00010003", { "C", 0 } },
        { "AC_00010004", { "LFE", 0 } }, { "AC_00010005", { "Ls", -110 } }, { "AC_00010006", { "Rs", 110 } } };
    auto it = tabla.find(ac);
    if (it == tabla.end()) return false;
    p.nombre = it->second.first; p.tipo = ac == "AC_00010004" ? Tipo::Lfe : Tipo::Altavoz; p.azimut.p = { { 0, it->second.second } };
    return true;
}

inline Archivo leeMetadatos(const std::string& xml, const std::vector<uint8_t>& chna, std::vector<std::vector<float>> canales, unsigned rate) {
    Nodo doc = LectorXml(xml).leer();
    std::map<std::string, std::map<std::string, const Nodo*>> idx; reune(doc, idx);
    if (chna.size() < 4) throw std::runtime_error("El bloque chna está vacío.");
    const unsigned uids = unsigned(chna[2] | chna[3] << 8);
    if (chna.size() < 4 + size_t(uids) * 40) throw std::runtime_error("El bloque chna está incompleto.");
    Archivo a; a.rate = rate;
    int conElevacion = 0, omitidas = 0;
    for (unsigned u = 0; u < uids; ++u) {
        const uint8_t* e = chna.data() + 4 + u * 40;
        const unsigned pista = unsigned(e[0] | e[1] << 8);
        auto cad = [&](size_t o, size_t n) { std::string s(reinterpret_cast<const char*>(e + o), n); return s.substr(0, s.find('\0')); };
        const std::string uid = cad(2, 12), at = cad(14, 14);
        if (pista == 0 || pista > canales.size()) continue;  // pista 0 = UID sin audio
        // AT → AS → AC por el XML; si no está, por convención (AT_xxxxxxxx_01 → AC_xxxxxxxx).
        std::string ac;
        if (auto t = idx["audioTrackFormat"][at]) if (auto rs = t->hijo("audioStreamFormatIDRef")) if (auto s = idx["audioStreamFormat"][rs->texto]) if (auto rc = s->hijo("audioChannelFormatIDRef")) ac = rc->texto;
        if (ac.empty()) if (auto t = idx["audioTrackUID"][uid]) if (auto rc = t->hijo("audioChannelFormatIDRef")) ac = rc->texto;
        if (ac.empty() && at.size() >= 11) ac = "AC_" + at.substr(3, 8);
        Pista p; p.audio = std::move(canales[pista - 1]);
        const Nodo* cf = idx["audioChannelFormat"][ac];
        if (!cf) {
            if (!definicionComun(ac, p)) { a.avisos += "Pista " + std::to_string(pista) + ": formato " + ac + " desconocido; se omite.\n"; ++omitidas; continue; }
            a.pistas.push_back(std::move(p)); continue;
        }
        std::string tl = cf->a("typeLabel"), td = cf->a("typeDefinition");
        p.nombre = cf->a("audioChannelFormatName"); if (p.nombre.empty()) p.nombre = ac;
        const bool objetos = tl == "0003" || td == "Objects", altavoz = tl == "0001" || td == "DirectSpeakers";
        if (!objetos && !altavoz) { a.avisos += "Pista " + std::to_string(pista) + " («" + p.nombre + "»): tipo " + (td.empty() ? tl : td) + " no soportado (solo Objects y DirectSpeakers); se omite.\n"; ++omitidas; continue; }
        std::vector<const Nodo*> bloques; for (auto& h : cf->hijos) if (h.nombre == "audioBlockFormat") bloques.push_back(&h);
        if (bloques.empty()) { a.avisos += "Pista " + std::to_string(pista) + ": sin bloques de posición; se omite.\n"; ++omitidas; continue; }
        double maxElev = 0;
        if (altavoz) {
            bool lfe = false;
            for (auto& h : bloques[0]->hijos) if (h.nombre == "speakerLabel" && (h.texto.find("LFE") != std::string::npos)) lfe = true;
            if (auto f = cf->hijo("frequency")) if (f->a("typeDefinition") == "lowPass" && std::strtod(f->texto.c_str(), nullptr) <= 200) lfe = true;
            double el = 0, az = azimutBloque(*bloques[0], el);
            p.tipo = lfe ? Tipo::Lfe : Tipo::Altavoz; p.azimut.p = { { 0, az } };
            if (!lfe) maxElev = std::fabs(el);
        } else {
            double anterior = 0; bool primero = true;
            for (const Nodo* b : bloques) {
                double el = 0, az = azimutBloque(*b, el); maxElev = std::max(maxElev, std::fabs(el));
                const double ini = leeTiempo(b->a("rtime")), dur = b->a("duration").empty() ? 0 : leeTiempo(b->a("duration"));
                double interp = dur;
                if (auto j = b->hijo("jumpPosition")) if (j->texto == "1" || j->texto == "true") interp = std::strtod(j->a("interpolationLength").c_str(), nullptr);
                if (primero) { p.azimut.p.push_back({ ini, az }); anterior = az; primero = false; continue; }
                az = anterior + std::remainder(az - anterior, 360.);  // por el camino corto
                // ADM: la posición anterior se mantiene hasta el inicio del bloque; ahí empieza a moverse
                // (durante la interpolación) o brinca (jumpPosition con interpolationLength 0).
                if (ini > p.azimut.p.back().first) p.azimut.p.push_back({ ini, anterior });
                p.azimut.p.push_back({ ini + std::max(0., std::min(interp, dur > 0 ? dur : interp)), az });
                anterior = az;
            }
            std::stable_sort(p.azimut.p.begin(), p.azimut.p.end(), [](auto& x, auto& y) { return x.first < y.first; });
        }
        if (maxElev > 1) ++conElevacion;
        a.pistas.push_back(std::move(p));
    }
    if (conElevacion) a.avisos += std::to_string(conElevacion) + " pista(s) traen elevación: se proyectan al anillo horizontal (el domo no tiene bocinas de altura).\n";
    if (a.pistas.empty()) throw std::runtime_error("El ADM no trae ninguna pista utilizable." + (omitidas ? " " + a.avisos : std::string()));
    return a;
}

// ---------- Archivo BW64/WAV ----------
inline void escribeArchivo(const std::string& ruta, const Archivo& a, const Metadatos& m) {
    const unsigned n = unsigned(a.pistas.size());
    const size_t frames = n ? a.pistas[0].audio.size() : 0;
    const uint64_t datos = uint64_t(frames) * n * 3;
    const uint64_t total = 4 + (8 + 16) + (8 + datos + (datos & 1)) + (8 + m.chna.size()) + (8 + m.xml.size() + (m.xml.size() & 1));
    if (total >= 0xFFFFFFFFull) throw std::runtime_error("El ADM pasaría de 4 GB: divide la escena.");
    std::ofstream f(std::filesystem::u8path(ruta), std::ios::binary);
    auto w16 = [&](uint32_t v) { f.put(char(v)); f.put(char(v >> 8)); };
    auto w32 = [&](uint32_t v) { w16(v & 0xFFFF); w16(v >> 16); };
    f.write("RIFF", 4); w32(uint32_t(total)); f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16); w16(1); w16(n); w32(a.rate); w32(a.rate * n * 3); w16(n * 3); w16(24);
    f.write("data", 4); w32(uint32_t(datos));
    std::vector<char> fila(size_t(n) * 3);
    for (size_t i = 0; i < frames; ++i) {
        for (unsigned c = 0; c < n; ++c) {
            int32_t s = int32_t(std::lround(std::clamp(double(a.pistas[c].audio[i]), -1., 1. - 1. / 8388608) * 8388608.));
            fila[c * 3] = char(s); fila[c * 3 + 1] = char(s >> 8); fila[c * 3 + 2] = char(s >> 16);
        }
        f.write(fila.data(), std::streamsize(fila.size()));
    }
    if (datos & 1) f.put(0);
    f.write("chna", 4); w32(uint32_t(m.chna.size())); f.write(reinterpret_cast<const char*>(m.chna.data()), std::streamsize(m.chna.size()));
    f.write("axml", 4); w32(uint32_t(m.xml.size())); f.write(m.xml.data(), std::streamsize(m.xml.size())); if (m.xml.size() & 1) f.put(0);
    if (!f) throw std::runtime_error("No se pudo escribir " + ruta);
}

// Lee un BW64/RF64/WAV con chna + axml. PCM 16/24/32 o flotante de 32 bit.
inline Archivo leeArchivo(const std::string& ruta) {
    std::ifstream f(std::filesystem::u8path(ruta), std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo abrir " + ruta);
    std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t o) { return uint32_t(uint8_t(d[o]) | uint8_t(d[o + 1]) << 8); };
    auto u32 = [&](size_t o) { return u16(o) | u16(o + 2) << 16; };
    auto u64 = [&](size_t o) { return uint64_t(u32(o)) | uint64_t(u32(o + 4)) << 32; };
    if (d.size() < 12 || std::memcmp(d.data() + 8, "WAVE", 4) || (std::memcmp(d.data(), "RIFF", 4) && std::memcmp(d.data(), "RF64", 4) && std::memcmp(d.data(), "BW64", 4)))
        throw std::runtime_error("No es un WAV/BW64.");
    unsigned fmt = 0, ch = 0, bits = 0, rate = 0; uint64_t dataGrande = 0; size_t data = 0; uint64_t len = 0;
    std::string xml; std::vector<uint8_t> chna;
    for (size_t o = 12; o + 8 <= d.size();) {
        uint64_t size = u32(o + 4);
        if (!std::memcmp(d.data() + o, "ds64", 4)) dataGrande = u64(o + 16);
        if (!std::memcmp(d.data() + o, "data", 4) && size == 0xFFFFFFFF) size = dataGrande;
        if (o + 8 + size > d.size()) size = d.size() - o - 8;
        if (!std::memcmp(d.data() + o, "fmt ", 4)) { fmt = u16(o + 8); ch = u16(o + 10); rate = u32(o + 12); bits = u16(o + 22); if (fmt == 0xFFFE) fmt = u16(o + 32); }
        if (!std::memcmp(d.data() + o, "data", 4)) { data = o + 8; len = size; }
        if (!std::memcmp(d.data() + o, "axml", 4)) xml.assign(d.data() + o + 8, size_t(size));
        if (!std::memcmp(d.data() + o, "chna", 4)) chna.assign(d.begin() + std::ptrdiff_t(o + 8), d.begin() + std::ptrdiff_t(o + 8 + size));
        o += 8 + size_t(size) + (size & 1);
    }
    if (xml.empty() || chna.empty()) throw std::runtime_error("El archivo no trae metadatos ADM (bloques axml y chna).");
    const bool flotante = fmt == 3 && bits == 32, pcm = fmt == 1 && (bits == 16 || bits == 24 || bits == 32);
    if ((!pcm && !flotante) || !ch) throw std::runtime_error("Formato de audio no soportado en el ADM.");
    const unsigned B = bits / 8; const size_t frames = size_t(len / (uint64_t(ch) * B));
    std::vector<std::vector<float>> canales(ch, std::vector<float>(frames));
    for (size_t i = 0; i < frames; ++i) for (unsigned c = 0; c < ch; ++c) {
        size_t o = data + (i * ch + c) * B; float v;
        if (flotante) { uint32_t u = u32(o); std::memcpy(&v, &u, 4); }
        else if (B == 2) v = int16_t(u16(o)) / 32768.f;
        else if (B == 3) v = float(int32_t(uint32_t(uint8_t(d[o])) << 8 | uint32_t(uint8_t(d[o + 1])) << 16 | uint32_t(uint8_t(d[o + 2])) << 24) / 2147483648.);
        else v = float(int32_t(u32(o)) / 2147483648.);
        canales[c][i] = v;
    }
    return leeMetadatos(xml, chna, std::move(canales), rate);
}

// ---------- Pistas → escena (importar) ----------
inline Escena aEscena(const Archivo& a, const std::string& nombre, const std::string& ficha) {
    Escena e; e.nombre = nombre; e.ficha = ficha;
    std::set<std::string> usados;
    for (const Pista& p : a.pistas) {
        Objeto o; o.nombre = p.nombre;
        for (int n = 2; !usados.insert(o.nombre).second; ++n) o.nombre = p.nombre + " " + std::to_string(n);
        o.mono = true; o.lfe = p.tipo == Tipo::Lfe;
        o.audio.rate = a.rate; o.audio.l = p.audio; o.audio.r = p.audio;
        o.mov.tipo = Movimiento::Puntos; o.mov.puntos = p.azimut.p.empty() ? Curva{ { { 0, 0 } } } : p.azimut;
        e.objetos.push_back(std::move(o));
    }
    return e;
}

}  // namespace pli::adm
