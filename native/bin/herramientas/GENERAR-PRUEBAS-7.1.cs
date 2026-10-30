using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
namespace PliPruebas {
  public static class Generador {
    const int Canales=8, Mascara=0x63F;
    const double Nivel=0.03162277660168379;
    static readonly int[] DomCanal={4,6,0,2,1,7,5};
    static readonly double[] DomAngulo={-145,-100,-30,0,30,100,145};
    enum Tipo { Tonos, Vuelta, Frente, Centro }

    public static string Ejecutar(string carpeta, int frecuencia) {
      if(frecuencia!=48000 && frecuencia!=96000) throw new ArgumentException("Usa 48000 o 96000 Hz.");
      Directory.CreateDirectory(carpeta);
      Crear(Path.Combine(carpeta,"01-Tonos-por-bocina-24bit-"+frecuencia+"Hz.wav"),frecuencia,Tipo.Tonos,25);
      Crear(Path.Combine(carpeta,"02-Vuelta-por-el-domo-24bit-"+frecuencia+"Hz.wav"),frecuencia,Tipo.Vuelta,24);
      Crear(Path.Combine(carpeta,"03-Estereo-frontal-L-R-24bit-"+frecuencia+"Hz.wav"),frecuencia,Tipo.Frente,12);
      Crear(Path.Combine(carpeta,"04-Centro-FC-24bit-"+frecuencia+"Hz.wav"),frecuencia,Tipo.Centro,8);
      return "Pruebas PCM 24 bit, 8 canales discretos, orden FL FR FC LFE BL BR SL SR. Nivel maximo: -30 dBFS.";
    }

    static void Cuatro(Stream s,string id) { byte[] b=Encoding.ASCII.GetBytes(id);s.Write(b,0,b.Length); }
    static void U16(BinaryWriter w,int n) { w.Write((ushort)n); }
    static void Crear(string ruta,int sr,Tipo tipo,int segundos) {
      long cuadros=(long)sr*segundos, bytesDatos=cuadros*Canales*3;
      using(FileStream fs=new FileStream(ruta,FileMode.CreateNew,FileAccess.Write,FileShare.None))
      using(BinaryWriter w=new BinaryWriter(fs)) {
        Cuatro(fs,"RIFF");w.Write((uint)(60+bytesDatos));Cuatro(fs,"WAVE");Cuatro(fs,"fmt ");w.Write(40u);
        U16(w,0xfffe);U16(w,Canales);w.Write((uint)sr);w.Write((uint)(sr*Canales*3));U16(w,Canales*3);U16(w,24);
        U16(w,22);U16(w,24);w.Write((uint)Mascara);w.Write(new Guid("00000001-0000-0010-8000-00aa00389b71").ToByteArray());
        Cuatro(fs,"data");w.Write((uint)bytesDatos);
        byte[] bloque=new byte[1024*Canales*3];long frame=0;int pos=0;
        double[] v=new double[Canales];
        while(frame<cuadros) {
          Array.Clear(v,0,Canales);
          if(tipo==Tipo.Tonos) {
            if(frame>=sr) {
              long relativo=frame-sr;int fase=(int)(relativo/(3L*sr));
              if(fase<Canales) {
                int c=fase;long local=relativo%(3L*sr);
                // Cada canal: 2 s tono, 1 s silencio; inicio general de 1 s.
                if(local<2L*sr) {
                  double t=local/(double)sr,env=Math.Min(1.0,Math.Min(t/0.05,(2.0-t)/0.05));
                  v[c]=Nivel*env*Math.Sin(2*Math.PI*(c==3?50:440)*t);
                }
              }
            }
          } else if(tipo==Tipo.Centro) {
            double t=frame/(double)sr;double env=Math.Min(1.0,Math.Min(t/0.05,(8.0-t)/0.05));
            v[2]=Nivel*env*Math.Sin(2*Math.PI*440*t);
          } else if(tipo==Tipo.Frente) {
            double t=frame/(double)sr;double env=Math.Min(1.0,Math.Min(t/0.05,(12.0-t)/0.05));
            v[0]=Nivel*env*Math.Sin(2*Math.PI*440*t);v[1]=Nivel*env*Math.Sin(2*Math.PI*660*t);
          } else {
            double t=frame/(double)sr,az=-180+360*t/24.0;
            Pan(az,v);double sen=Nivel*Math.Sin(2*Math.PI*440*t);
            for(int c=0;c<Canales;c++)v[c]*=sen;
          }
          for(int c=0;c<Canales;c++) {
            int sample=(int)Math.Round(Math.Max(-1,Math.Min(1,v[c]))*8388607.0);
            uint q=unchecked((uint)sample);
            bloque[pos++]=(byte)q;bloque[pos++]=(byte)(q>>8);bloque[pos++]=(byte)(q>>16);
          }
          frame++;
          if(pos==bloque.Length){w.Write(bloque,0,pos);pos=0;}
        }
        if(pos>0)w.Write(bloque,0,pos);
      }
    }
    static void Pan(double az,double[] g) {
      while(az<DomAngulo[0])az+=360;while(az>=DomAngulo[0]+360)az-=360;
      int k=DomAngulo.Length-1;
      for(int i=0;i<DomAngulo.Length-1;i++)if(az>=DomAngulo[i]&&az<DomAngulo[i+1]){k=i;break;}
      double a=DomAngulo[k],b=k==DomAngulo.Length-1?DomAngulo[0]+360:DomAngulo[k+1];
      double f=(az-a)/(b-a),x=Math.Cos(f*Math.PI/2),y=Math.Sin(f*Math.PI/2);
      double norm=Math.Sqrt(x*x+y*y);g[DomCanal[k]]=x/norm;g[DomCanal[(k+1)%DomAngulo.Length]]=y/norm;
    }
  }
}
