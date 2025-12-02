# ⚡ Inicio Rápido - Ejecutar en Celular

## 🎯 La Solución Más Simple

Ejecuta la app directamente en tu celular. Como tu celular y Arduino están en la misma WiFi, **no necesitas ngrok ni Replit**.

## 📱 Pasos Rápidos (10 minutos)

### 1️⃣ Instalar Termux

- Descarga desde [Google Play](https://play.google.com/store/apps/details?id=com.termux) o [F-Droid](https://f-droid.org/en/packages/com.termux/)

### 2️⃣ Instalar Node.js

Abre Termux y ejecuta:
```bash
pkg update
pkg install nodejs git
```

### 3️⃣ Subir tu Proyecto

**Opción A - Desde GitHub:**
```bash
cd ~
git clone TU_URL_DE_GITHUB
cd app-riego
```

**Opción B - Transferir ZIP:**
1. Comprime tu proyecto en PC
2. Transfiere a tu celular
3. En Termux:
```bash
cd ~/storage/downloads
unzip app-riego.zip
cd app-riego
```

### 4️⃣ Instalar y Ejecutar

```bash
npm install
npm start
```

### 5️⃣ Abrir en Navegador

- En el mismo celular: `http://localhost:3000`
- Desde otro dispositivo: `http://IP_DE_TU_CELULAR:3000`

### 6️⃣ Configurar Arduino

- Toca ⚙️ en la app
- Ingresa la IP local de tu Arduino (ej: `192.168.1.100`)
- ¡Listo! 🎉

## ✅ Ventajas

- ✅ No necesitas Replit
- ✅ No necesitas ngrok
- ✅ Conexión directa (más rápido)
- ✅ Funciona offline
- ✅ Gratis

## 📖 Guía Completa

Ver `EJECUTAR_EN_CELULAR.md` para detalles y solución de problemas.

