# ⚡ Inicio Rápido - Replit + Túnel

## 🎯 Configuración en 5 minutos

### 1️⃣ Exponer tu Arduino (Elige el método más fácil)

#### ⭐ Método Más Fácil: Serveo (Sin Instalación)

1. **Abre tu navegador** en tu celular (misma WiFi que Arduino)
2. **Ve a**: https://serveo.net
3. **Ejecuta** (reemplaza `192.168.1.50` con la IP de tu Arduino):
   ```bash
   ssh -R 80:192.168.1.50:80 serveo.net
   ```
4. **Copia la URL** que aparece (ej: `https://abc123.serveo.net`)

#### Método Alternativo: ngrok en PC

Si tienes una PC en la misma red:
1. Descarga ngrok desde [ngrok.com/download](https://ngrok.com/download)
2. Crea cuenta y obtén tu token
3. Ejecuta: `ngrok authtoken TU_TOKEN` luego `ngrok http 80`
4. Copia la URL que aparece

**Ver más opciones en**: `ALTERNATIVAS_NGROK.md`

### 2️⃣ Subir a Replit

1. Ve a [replit.com](https://replit.com)
2. Crea nuevo Repl → Importa tu proyecto
3. Haz clic en "Run"

### 3️⃣ Conectar

1. En la app de Replit, toca ⚙️
2. Pega la URL de ngrok en "IP o URL del NodeMCU"
3. ¡Listo! 🎉

## ⚠️ Recordatorio

- **Mantén ngrok ejecutándose** mientras uses la app
- Si reinicias ngrok, **actualiza la URL** en la app
- Tu celular y Arduino deben estar en la **misma WiFi**

## 📖 Guía Completa

Ver `REPLIT_NGROK.md` para más detalles y solución de problemas.

