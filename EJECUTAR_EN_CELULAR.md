# 📱 Ejecutar la App Directamente en tu Celular

Esta es la solución más simple: ejecutar la app React directamente en tu celular. Como tu celular y Arduino están en la misma WiFi, puedes conectarte directamente sin necesidad de ngrok o Replit.

## 🎯 Ventajas

- ✅ **No necesitas Replit** - Todo corre en tu celular
- ✅ **No necesitas ngrok** - Conexión directa a través de la WiFi local
- ✅ **Más rápido** - Sin latencia de internet
- ✅ **Funciona offline** - Solo necesitas WiFi local
- ✅ **Gratis** - No requiere servicios externos

## 📋 Requisitos

- Android (iOS es más complicado, pero también posible)
- Aplicación **Termux** (terminal para Android)
- Al menos 500MB de espacio libre
- Conexión WiFi (para descargar dependencias la primera vez)

## 🚀 Paso 1: Instalar Termux

1. **Descarga Termux** desde:
   - [Google Play Store](https://play.google.com/store/apps/details?id=com.termux)
   - O desde [F-Droid](https://f-droid.org/en/packages/com.termux/) (recomendado)

2. **Abre Termux** en tu celular

## 🔧 Paso 2: Instalar Node.js y Git

En Termux, ejecuta estos comandos uno por uno:

```bash
# Actualizar paquetes
pkg update && pkg upgrade

# Instalar Node.js, Git y herramientas necesarias
pkg install nodejs git wget curl

# Verificar instalación
node --version
npm --version
```

## 📥 Paso 3: Subir tu Proyecto al Celular

Tienes varias opciones:

### Opción A: Desde GitHub (Recomendado)

1. **Sube tu proyecto a GitHub** (si no lo tienes):
   ```bash
   # En tu PC, en la carpeta del proyecto:
   git init
   git add .
   git commit -m "Initial commit"
   git remote add origin TU_URL_DE_GITHUB
   git push -u origin main
   ```

2. **En Termux, clona el proyecto**:
   ```bash
   cd ~
   git clone TU_URL_DE_GITHUB
   cd app-riego
   ```

### Opción B: Transferir Archivos Manualmente

1. **Comprime tu proyecto** en tu PC (ZIP)
2. **Transfiere el ZIP** a tu celular (USB, email, Google Drive, etc.)
3. **En Termux, descomprime**:
   ```bash
   cd ~
   # Si está en Downloads:
   cd storage/downloads
   unzip app-riego.zip
   cd app-riego
   ```

### Opción C: Usar `scp` o `rsync` desde PC

Si tienes acceso SSH:
```bash
# Desde tu PC:
scp -r app-riego usuario@IP_CELULAR:~/app-riego
```

## 📦 Paso 4: Instalar Dependencias

En Termux, dentro de la carpeta del proyecto:

```bash
# Asegúrate de estar en la carpeta del proyecto
cd ~/app-riego

# Instalar dependencias (esto puede tardar varios minutos)
npm install
```

**Nota**: La primera vez puede tardar 5-10 minutos. Ten paciencia.

## ▶️ Paso 5: Ejecutar la App

### Opción A: Modo Desarrollo (con hot-reload)

```bash
npm start
```

Esto iniciará el servidor de desarrollo. Verás algo como:
```
Compiled successfully!

You can now view app-riego in the browser.

  Local:            http://localhost:3000
  On Your Network:  http://192.168.1.XXX:3000
```

### Opción B: Compilar y Servir (Producción)

Si prefieres una versión optimizada:

```bash
# Compilar la app
npm run build

# Instalar un servidor simple
npm install -g serve

# Servir la app compilada
serve -s build -l 3000
```

## 🌐 Paso 6: Acceder a la App

Tienes dos opciones:

### Opción 1: Desde el mismo celular

1. **Abre tu navegador** en el celular
2. **Ve a**: `http://localhost:3000`
3. **¡Listo!** La app debería cargar

### Opción 2: Desde otro dispositivo (PC, otro celular)

1. **Anota la IP de tu celular**:
   - En Termux, ejecuta: `ifconfig` o `ip addr show`
   - Busca la IP en la red WiFi (ej: `192.168.1.50`)

2. **Desde tu PC u otro dispositivo** (misma WiFi):
   - Abre el navegador
   - Ve a: `http://192.168.1.50:3000`
   - (Reemplaza con la IP de tu celular)

## ⚙️ Paso 7: Configurar la IP del Arduino

1. **En la app**, toca el ícono de configuración (⚙️)
2. **Ingresa la IP local de tu Arduino**:
   - Ejemplo: `192.168.1.100`
   - (No necesitas https:// ni ngrok, solo la IP local)
3. **La app se conectará directamente** a través de la WiFi local

## 🔄 Mantener la App Ejecutándose

### Problema: Se cierra al cerrar Termux

**Solución**: Usa `tmux` o `screen` para mantener sesiones:

```bash
# Instalar tmux
pkg install tmux

# Iniciar sesión tmux
tmux

# Dentro de tmux, ejecuta tu app
npm start

# Para salir de tmux (sin cerrar la app): Ctrl+B, luego D
# Para volver a tmux: tmux attach
```

### Ejecutar en segundo plano

```bash
# Ejecutar en background
nohup npm start > app.log 2>&1 &

# Ver logs
tail -f app.log

# Detener
pkill -f "react-scripts"
```

## 🔋 Optimización de Batería

Para ahorrar batería:

1. **Compila la app** (`npm run build`) y usa `serve` en lugar de `npm start`
2. **Reduce el auto-refresh** en la app (cambia de 3 segundos a 10-30 segundos)
3. **Cierra Termux** cuando no uses la app (pero mantén la sesión tmux activa)

## 🐛 Solución de Problemas

### Error: "Permission denied"
```bash
# Dar permisos de almacenamiento
termux-setup-storage
```

### Error: "Port 3000 already in use"
```bash
# Usar otro puerto
PORT=3001 npm start
```

### Error: "Cannot find module"
```bash
# Reinstalar dependencias
rm -rf node_modules package-lock.json
npm install
```

### La app no carga
- Verifica que el servidor esté ejecutándose
- Verifica que uses la IP correcta
- Asegúrate de estar en la misma WiFi
- Prueba `http://localhost:3000` primero

### No puedo acceder desde otro dispositivo
- Verifica que ambos dispositivos estén en la misma WiFi
- Verifica el firewall de Android (puede bloquear conexiones)
- Prueba desactivar temporalmente el firewall/VPN

## 📱 Acceso Rápido

Para acceder fácilmente:

1. **Crea un bookmark** en tu navegador con la URL
2. **O agrega a pantalla de inicio** (funciona como PWA)
3. **O crea un script** en Termux:

```bash
# Crear script de inicio rápido
echo 'cd ~/app-riego && npm start' > ~/start-riego.sh
chmod +x ~/start-riego.sh

# Para ejecutar:
~/start-riego.sh
```

## 🎯 Flujo de Trabajo Diario

1. **Abre Termux** en tu celular
2. **Ejecuta**: `cd ~/app-riego && npm start`
3. **Abre tu navegador**: `http://localhost:3000`
4. **¡Listo!** Controla tu sistema de riego

## 💡 Consejos

- **Primera vez**: Compila la app (`npm run build`) para mejor rendimiento
- **Uso diario**: Usa `npm start` para desarrollo o `serve -s build` para producción
- **Mantén Termux actualizado**: `pkg update && pkg upgrade`
- **Guarda la IP del Arduino** en la configuración de la app para no tener que ingresarla cada vez

---

¡Ahora tienes tu app de Riego Inteligente ejecutándose directamente en tu celular! 🌱💧

