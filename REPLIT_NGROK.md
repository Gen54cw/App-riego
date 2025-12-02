# 🚀 Configuración para Replit con ngrok

Esta guía te ayudará a ejecutar tu app de Riego Inteligente en Replit y conectarla a tu Arduino desde cualquier lugar usando ngrok.

## 📋 Situación

- **Tu PC**: Está en otra red WiFi
- **Tu Celular y Arduino**: Comparten la misma red WiFi
- **Solución**: Usar Replit para la app + ngrok para exponer el Arduino

## 🔧 Paso 1: Configurar ngrok en tu Celular/PC (misma red que Arduino)

### Opción A: Instalar ngrok en tu Celular (Android)

1. **Descarga ngrok** desde [ngrok.com/download](https://ngrok.com/download) o desde Google Play Store
2. **Crea una cuenta gratuita** en [ngrok.com](https://ngrok.com) (gratis)
3. **Obtén tu token de autenticación** desde el dashboard de ngrok
4. **Configura ngrok**:
   ```bash
   ngrok authtoken TU_TOKEN_AQUI
   ngrok http 80
   ```
   (Ajusta el puerto si tu Arduino usa otro, normalmente 80)

5. **Copia la URL HTTPS** que aparece (ej: `https://abc123.ngrok-free.app`)

### Opción B: Instalar ngrok en una PC en la misma red

Si tienes acceso a una PC en la misma red WiFi que el Arduino:

1. **Descarga ngrok** para Windows/Mac/Linux desde [ngrok.com/download](https://ngrok.com/download)
2. **Sigue los mismos pasos** que en la Opción A
3. **Mantén ngrok ejecutándose** mientras uses la app

## 🌐 Paso 2: Subir tu Proyecto a Replit

1. **Ve a [Replit](https://replit.com)** y crea una cuenta (gratis)
2. **Crea un nuevo Repl**:
   - Haz clic en "Create Repl"
   - Selecciona "Import from GitHub" si tienes el código en GitHub
   - O selecciona "Node.js" y sube los archivos manualmente

3. **Asegúrate de tener estos archivos**:
   - `.replit` (ya creado)
   - `.replit.nix` (ya creado)
   - `package.json`
   - `src/App.js`
   - Todos los demás archivos del proyecto

4. **Replit instalará automáticamente las dependencias** cuando detecte el `package.json`

## ▶️ Paso 3: Ejecutar la App en Replit

1. **Haz clic en el botón "Run"** en Replit
2. **O ejecuta manualmente**: `npm start`
3. **Replit abrirá automáticamente** la aplicación en una ventana

## 🔗 Paso 4: Conectar la App con tu Arduino

1. **En la app de Replit**, haz clic en el ícono de configuración (⚙️)
2. **En el campo "IP o URL del NodeMCU"**, pega la URL de ngrok que copiaste:
   - Ejemplo: `https://abc123.ngrok-free.app`
   - O simplemente: `abc123.ngrok-free.app` (la app agregará https:// automáticamente)

3. **La app se conectará automáticamente** a tu Arduino a través de ngrok

## ✅ Verificación

- Deberías ver los datos del Arduino en la app
- Los sensores deberían actualizarse cada 3 segundos
- Puedes controlar el riego desde Replit

## ⚠️ Notas Importantes

### Sobre ngrok (Plan Gratuito)

- **La URL cambia cada vez que reinicias ngrok**
- **Solución**: Cada vez que reinicies ngrok, copia la nueva URL y actualízala en la app
- **Para URL fija**: Usa Cloudflare Tunnel (gratis) o el plan de pago de ngrok

### Mantener ngrok Activo

- **ngrok debe estar ejecutándose** mientras uses la app
- Si cierras ngrok, la app perderá la conexión
- Puedes minimizar la ventana de ngrok, pero no la cierres

### Seguridad

- La URL de ngrok es pública (cualquiera que la conozca puede acceder)
- **Recomendación**: No compartas la URL públicamente
- Considera agregar autenticación básica en tu Arduino si es crítico

## 🔄 Flujo de Trabajo Diario

1. **Enciende tu Arduino** (debe estar en la misma WiFi que tu celular)
2. **Inicia ngrok** en tu celular/PC (misma red que Arduino):
   ```bash
   ngrok http 80
   ```
3. **Copia la nueva URL** de ngrok
4. **Abre tu app en Replit**
5. **Actualiza la URL** en la configuración si cambió
6. **¡Listo!** Ya puedes controlar tu sistema de riego desde cualquier lugar

## 🐛 Solución de Problemas

### Error: "No se pudo conectar al NodeMCU"
- ✅ Verifica que ngrok esté ejecutándose
- ✅ Verifica que la URL sea correcta (sin espacios)
- ✅ Asegúrate de que el Arduino esté encendido
- ✅ Verifica que el puerto en ngrok coincida con el del Arduino

### Error: CORS (Cross-Origin Resource Sharing)
- Tu Arduino debe permitir peticiones desde cualquier origen
- En tu código Arduino, agrega estas cabeceras:
  ```cpp
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  ```

### La URL de ngrok cambia constantemente
- **Solución temporal**: Actualiza la URL en la app cada vez
- **Solución permanente**: Usa Cloudflare Tunnel (gratis, URL fija)

## 📱 Acceso desde tu Celular

Una vez configurado en Replit:
- **Abre Replit en tu celular** (navegador móvil)
- **O comparte la URL de Replit** contigo mismo
- La app funcionará perfectamente en móvil también

## 💡 Alternativa: Cloudflare Tunnel (URL Fija)

Si quieres una URL que no cambie:

1. **Instala cloudflared** en tu celular/PC
2. **Crea una cuenta** en Cloudflare (gratis)
3. **Ejecuta**:
   ```bash
   cloudflared tunnel --url http://192.168.1.50:80
   ```
4. **Obtén la URL** que Cloudflare te da (será fija)
5. **Úsala en la app** igual que con ngrok

---

¡Listo! Ahora puedes controlar tu sistema de riego desde cualquier lugar usando Replit + ngrok. 🌱💧

