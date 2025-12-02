import React, { useState, useEffect, useCallback } from 'react';
import { RefreshCw, Droplet, Thermometer, Wind, Sprout, Activity, Settings, AlertTriangle, Play, Square, BarChart3, Save } from 'lucide-react';

export default function AppRiegoAutonomico() {
  // Estado de la app
  const [ipAddress, setIpAddress] = useState('192.168.18.50');
  const [data, setData] = useState(null);
  const [cultivos, setCultivos] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [showConfig, setShowConfig] = useState(false);
  const [lastUpdate, setLastUpdate] = useState(new Date());
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [notificaciones, setNotificaciones] = useState([]);
  const [comandoEjecutando, setComandoEjecutando] = useState(null);

  // Función helper para normalizar la URL/IP
  const normalizeUrl = useCallback((address) => {
    if (!address) return '';
    // Si ya tiene http:// o https://, devolverlo tal cual
    if (address.startsWith('http://') || address.startsWith('https://')) {
      return address;
    }
    // Si es una URL de ngrok/cloudflare, usar https
    if (address.includes('.ngrok.io') || address.includes('.ngrok-free.app') || 
        address.includes('.cloudflared.com') || address.includes('ngrok')) {
      return `https://${address}`;
    }
    // Para IPs locales, usar http
    return `http://${address}`;
  }, []);

  // Función para obtener datos del NodeMCU
  const fetchData = useCallback(async () => {
    try {
      setLoading(true);
      const baseUrl = normalizeUrl(ipAddress);
      const response = await fetch(`${baseUrl}/api/data`);
      if (!response.ok) throw new Error('No se pudo conectar al NodeMCU');
      const result = await response.json();
      setData(result);
      setError(null);
      setLastUpdate(new Date());
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }, [ipAddress, normalizeUrl]);

  // Función para obtener lista de cultivos
  const fetchCultivos = useCallback(async () => {
    try {
      const baseUrl = normalizeUrl(ipAddress);
      const response = await fetch(`${baseUrl}/api/cultivos`);
      if (!response.ok) throw new Error('Error al obtener cultivos');
      const result = await response.json();
      setCultivos(result);
    } catch (err) {
      console.error('Error cultivos:', err);
    }
  }, [ipAddress, normalizeUrl]);

  // Función para cambiar cultivo
  const changeCultivo = async (cultivoId) => {
    try {
      // Deshabilitar auto-refresh temporalmente para evitar que se sobrescriban los valores
      const wasAutoRefresh = autoRefresh;
      setAutoRefresh(false);
      
      const baseUrl = normalizeUrl(ipAddress);
      const response = await fetch(`${baseUrl}/api/cultivo?id=${cultivoId}`, {
        method: 'POST',
      });
      if (!response.ok) throw new Error('Error al cambiar cultivo');
      
      // Esperar un momento para que el servidor procese el cambio
      await new Promise(resolve => setTimeout(resolve, 500));
      
      // Actualizar datos
      await fetchData();
      
      // Esperar un poco más y actualizar nuevamente para obtener los valores del nuevo cultivo
      setTimeout(async () => {
        await fetchData();
        // Rehabilitar auto-refresh si estaba activo
        if (wasAutoRefresh) {
          setAutoRefresh(true);
        }
      }, 1000);
      
      const cultivo = cultivos.find(c => c.id === cultivoId);
      agregarNotificacion(
        'Cultivo actualizado', 
        'success', 
        `Sistema configurado para: ${cultivo?.nombre || 'Cultivo ' + cultivoId}. Los parámetros de riego se ajustarán automáticamente.`
      );
    } catch (err) {
      agregarNotificacion('Error al cambiar cultivo', 'error', err.message);
      // Rehabilitar auto-refresh en caso de error
      setAutoRefresh(true);
    }
  };

  // Función para agregar notificación
  const agregarNotificacion = (mensaje, tipo = 'info', detalles = '') => {
    const nuevaNotificacion = {
      id: Date.now(),
      mensaje,
      detalles,
      tipo, // 'success', 'error', 'warning', 'info'
      timestamp: new Date()
    };
    setNotificaciones(prev => [nuevaNotificacion, ...prev].slice(0, 5)); // Máximo 5 notificaciones
    
    // Auto-eliminar después de 5 segundos
    setTimeout(() => {
      setNotificaciones(prev => prev.filter(n => n.id !== nuevaNotificacion.id));
    }, 5000);
  };

  // Función para enviar comandos
  const sendCommand = async (command) => {
    if (comandoEjecutando) {
      agregarNotificacion('Espere, hay un comando en ejecución', 'warning');
      return;
    }

    setComandoEjecutando(command);
    agregarNotificacion('Enviando comando...', 'info');

    try {
      const baseUrl = normalizeUrl(ipAddress);
      const response = await fetch(`${baseUrl}/api/command?cmd=${command}`, {
        method: 'POST',
      });
      
      if (!response.ok) {
        throw new Error('Error al conectar con el sistema');
      }
      
      const result = await response.json();
      
      if (result.status === 'ok') {
        agregarNotificacion(result.message, 'success', result.detalles || '');
      } else {
        agregarNotificacion(result.message, 'warning', result.detalles || '');
      }
      
      // Actualizar datos después de un breve delay
      setTimeout(() => {
        fetchData();
      }, 500);
      
    } catch (err) {
      agregarNotificacion('Error de conexión', 'error', 'No se pudo conectar con el sistema. Verifique la IP y la conexión WiFi.');
      setError(err.message);
    } finally {
      setComandoEjecutando(null);
    }
  };

  // Auto-refresh cada 3 segundos
  useEffect(() => {
    fetchData();
    fetchCultivos();
    
    let interval;
    if (autoRefresh) {
      interval = setInterval(fetchData, 3000);
    }
    
    return () => {
      if (interval) clearInterval(interval);
    };
  }, [autoRefresh, fetchData, fetchCultivos]);

  // Función para calcular color según humedad
  const getHumidityColor = (humidity) => {
    if (humidity < 40) return '#ef4444';
    if (humidity < 60) return '#f59e0b';
    return '#10b981';
  };

  // Componente de Configuración Manual
  const ConfiguracionManual = ({ data, ipAddress, onConfigChange, agregarNotificacion, normalizeUrl }) => {
    const [config, setConfig] = useState({
      umbralHumedadMin: data?.umbralHumedadMin || 60,
      umbralHumedadOptimo: data?.umbralHumedadOptimo || 70,
      umbralHumedadMax: data?.umbralHumedadMax || 85,
      tiempoRiego: data?.tiempoRiegoActual || 30,
      tiempoMaxRiego: data?.tiempoMaxRiego || 300,
      tiempoMinEntreRiegos: data?.tiempoMinimoEntreRiegos || 60,
      tempMaxSegura: 45.0,
      tempMinSegura: 0.0
    });
    const [guardando, setGuardando] = useState(false);
    const [editando, setEditando] = useState(false);
    const [ultimaActualizacion, setUltimaActualizacion] = useState(Date.now());

    // Rastrear si el usuario está editando activamente
    const handleInputFocus = () => {
      setEditando(true);
    };

    const handleInputBlur = () => {
      // Esperar un momento antes de permitir actualizaciones automáticas
      setTimeout(() => {
        setEditando(false);
      }, 500);
    };

    useEffect(() => {
      if (data && !guardando && !editando) {
        // Solo actualizar si no estamos guardando, editando, y han pasado al menos 2 segundos desde la última actualización
        const ahora = Date.now();
        if (ahora - ultimaActualizacion > 2000) {
          setConfig(prevConfig => {
            // Verificar si realmente hay cambios antes de actualizar
            const hayCambios = 
              (data.umbralHumedadMin !== undefined && data.umbralHumedadMin !== prevConfig.umbralHumedadMin) ||
              (data.umbralHumedadOptimo !== undefined && data.umbralHumedadOptimo !== prevConfig.umbralHumedadOptimo) ||
              (data.umbralHumedadMax !== undefined && data.umbralHumedadMax !== prevConfig.umbralHumedadMax) ||
              (data.tiempoRiegoActual !== undefined && data.tiempoRiegoActual !== prevConfig.tiempoRiego) ||
              (data.tiempoMaxRiego !== undefined && data.tiempoMaxRiego !== prevConfig.tiempoMaxRiego) ||
              (data.tiempoMinimoEntreRiegos !== undefined && data.tiempoMinimoEntreRiegos !== prevConfig.tiempoMinEntreRiegos);
            
            if (hayCambios) {
              setUltimaActualizacion(ahora);
              return {
                umbralHumedadMin: data.umbralHumedadMin ?? prevConfig.umbralHumedadMin ?? 60,
                umbralHumedadOptimo: data.umbralHumedadOptimo ?? prevConfig.umbralHumedadOptimo ?? 70,
                umbralHumedadMax: data.umbralHumedadMax ?? prevConfig.umbralHumedadMax ?? 85,
                tiempoRiego: data.tiempoRiegoActual ?? prevConfig.tiempoRiego ?? 30,
                tiempoMaxRiego: data.tiempoMaxRiego ?? prevConfig.tiempoMaxRiego ?? 300,
                tiempoMinEntreRiegos: data.tiempoMinimoEntreRiegos ?? prevConfig.tiempoMinEntreRiegos ?? 60,
                tempMaxSegura: prevConfig.tempMaxSegura ?? 45.0,
                tempMinSegura: prevConfig.tempMinSegura ?? 0.0
              };
            }
            return prevConfig;
          });
        }
      }
    }, [data, guardando, editando, ultimaActualizacion]);

    const handleSave = async () => {
      setGuardando(true);
      setEditando(false); // Ya no estamos editando
      try {
        const params = new URLSearchParams({
          umbralMin: config.umbralHumedadMin.toString(),
          umbralOptimo: config.umbralHumedadOptimo.toString(),
          umbralMax: config.umbralHumedadMax.toString(),
          tiempoRiego: config.tiempoRiego.toString(),
          tiempoMaxRiego: config.tiempoMaxRiego.toString(),
          tiempoMinEntreRiegos: config.tiempoMinEntreRiegos.toString(),
          tempMax: config.tempMaxSegura.toString(),
          tempMin: config.tempMinSegura.toString()
        });

        const baseUrl = normalizeUrl(ipAddress);
        const response = await fetch(`${baseUrl}/api/config?${params}`, {
          method: 'POST',
        });

        if (!response.ok) {
          const errorText = await response.text();
          throw new Error(errorText || 'Error al guardar configuración');
        }
        
        const result = await response.json();
        agregarNotificacion('Configuración guardada', 'success', result.detalles || 'Los parámetros se han actualizado correctamente');
        
        // Actualizar timestamp para permitir actualizaciones automáticas
        setUltimaActualizacion(Date.now());
        
        // Esperar un momento para que el servidor procese el cambio
        await new Promise(resolve => setTimeout(resolve, 500));
        
        // Actualizar los datos
        await onConfigChange();
        
      } catch (err) {
        agregarNotificacion('Error al guardar', 'error', err.message);
      } finally {
        setGuardando(false);
      }
    };

    return (
      <div className="space-y-4">
        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Humedad Mínima (%)
            </label>
            <input
              type="number"
              min="0"
              max="100"
              value={config.umbralHumedadMin}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, umbralHumedadMin: parseInt(e.target.value) || 0});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
          
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Humedad Óptima (%)
            </label>
            <input
              type="number"
              min="0"
              max="100"
              value={config.umbralHumedadOptimo}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, umbralHumedadOptimo: parseInt(e.target.value) || 0});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
          
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Humedad Máxima (%)
            </label>
            <input
              type="number"
              min="0"
              max="100"
              value={config.umbralHumedadMax}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, umbralHumedadMax: parseInt(e.target.value) || 0});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
          
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Tiempo Riego (s)
            </label>
            <input
              type="number"
              min="10"
              max="600"
              value={config.tiempoRiego}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, tiempoRiego: parseInt(e.target.value) || 30});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
          
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Tiempo Máx Riego (s)
            </label>
            <input
              type="number"
              min="60"
              max="1800"
              value={config.tiempoMaxRiego}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, tiempoMaxRiego: parseInt(e.target.value) || 300});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
          
          <div>
            <label className="block text-xs font-semibold text-gray-700 mb-1">
              Tiempo Mín Entre Riegos (s)
            </label>
            <input
              type="number"
              min="30"
              max="600"
              value={config.tiempoMinEntreRiegos}
              onChange={(e) => {
                setEditando(true);
                setConfig({...config, tiempoMinEntreRiegos: parseInt(e.target.value) || 60});
              }}
              onFocus={handleInputFocus}
              onBlur={handleInputBlur}
              className="w-full px-3 py-2 border-2 border-gray-300 rounded-lg focus:outline-none focus:border-blue-500 text-sm"
            />
          </div>
        </div>
        
        <button
          onClick={handleSave}
          disabled={guardando}
          className={`w-full flex items-center justify-center gap-2 bg-gradient-to-r from-green-500 to-emerald-500 text-white py-3 rounded-xl font-semibold hover:shadow-lg transition ${
            guardando ? 'opacity-50 cursor-not-allowed' : ''
          }`}
        >
          {guardando ? (
            <RefreshCw size={18} className="animate-spin" />
          ) : (
            <Save size={18} />
          )}
          {guardando ? 'Guardando...' : 'Guardar Configuración'}
        </button>
      </div>
    );
  };

  // Renderizado de la app
  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-500 via-purple-500 to-pink-500 p-4">
      <div className="max-w-md mx-auto">
        {/* Notificaciones */}
        <div className="fixed top-4 right-4 z-50 space-y-2 max-w-sm">
          {notificaciones.map((notif) => (
            <div
              key={notif.id}
              className={`rounded-lg shadow-2xl p-4 animate-slide-in ${
                notif.tipo === 'success' ? 'bg-green-500 text-white' :
                notif.tipo === 'error' ? 'bg-red-500 text-white' :
                notif.tipo === 'warning' ? 'bg-yellow-500 text-white' :
                'bg-blue-500 text-white'
              }`}
            >
              <div className="font-semibold">{notif.mensaje}</div>
              {notif.detalles && (
                <div className="text-sm mt-1 opacity-90">{notif.detalles}</div>
              )}
            </div>
          ))}
        </div>
        {/* Header */}
        <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center gap-3">
              <div className="w-12 h-12 bg-gradient-to-br from-green-400 to-blue-500 rounded-full flex items-center justify-center">
                <Sprout className="text-white" size={24} />
              </div>
              <div>
                <h1 className="text-2xl font-bold text-gray-800">Riego Inteligente</h1>
                <p className="text-sm text-gray-500">Sistema Autonómico</p>
              </div>
            </div>
            <button
              onClick={() => setShowConfig(!showConfig)}
              className="w-10 h-10 bg-gray-100 rounded-full flex items-center justify-center hover:bg-gray-200 transition"
            >
              <Settings size={20} className="text-gray-600" />
            </button>
          </div>

          {/* Panel de configuración */}
          {showConfig && (
            <div className="bg-blue-50 rounded-xl p-4 mb-4">
              <label className="block text-sm font-semibold text-gray-700 mb-2">
                IP o URL del NodeMCU
              </label>
              <input
                type="text"
                value={ipAddress}
                onChange={(e) => setIpAddress(e.target.value)}
                className="w-full px-4 py-2 border-2 border-blue-300 rounded-lg focus:outline-none focus:border-blue-500"
                placeholder="192.168.1.100 o https://abc123.ngrok-free.app"
              />
              <p className="text-xs text-gray-600 mt-2">
                💡 Para acceso remoto desde Replit, usa una URL de ngrok o Cloudflare Tunnel
              </p>
              <div className="mt-3 flex items-center justify-between">
                <label className="text-sm font-medium text-gray-700">
                  Actualización automática
                </label>
                <button
                  onClick={() => setAutoRefresh(!autoRefresh)}
                  className={`w-12 h-6 rounded-full transition ${
                    autoRefresh ? 'bg-green-500' : 'bg-gray-300'
                  }`}
                >
                  <div
                    className={`w-5 h-5 bg-white rounded-full shadow transform transition ${
                      autoRefresh ? 'translate-x-6' : 'translate-x-0.5'
                    }`}
                  />
                </button>
              </div>
            </div>
          )}

          {/* Estado de conexión */}
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <div className={`w-3 h-3 rounded-full animate-pulse ${error ? 'bg-red-500' : 'bg-green-500'}`} />
              <span className="text-sm font-medium text-gray-600">
                {error ? 'Desconectado' : 'Conectado'}
              </span>
            </div>
            <button
              onClick={fetchData}
              disabled={loading}
              className="flex items-center gap-2 px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 transition disabled:opacity-50"
            >
              <RefreshCw size={16} className={loading ? 'animate-spin' : ''} />
              <span className="text-sm font-medium">Actualizar</span>
            </button>
          </div>

          {error && (
            <div className="mt-3 bg-red-50 border-l-4 border-red-500 p-3 rounded">
              <p className="text-sm text-red-700">{error}</p>
            </div>
          )}
        </div>

        {data && (
          <>
            {/* Estado del Sistema */}
            <div className={`rounded-3xl shadow-2xl p-6 mb-4 text-white ${
              data.estado === 'Riego Activo' ? 'bg-gradient-to-r from-blue-500 to-cyan-500' :
              data.estado === 'Emergencia' ? 'bg-gradient-to-r from-red-500 to-pink-500 animate-pulse' :
              'bg-gradient-to-r from-green-500 to-emerald-500'
            }`}>
              <div className="text-center">
                <div className="text-4xl mb-2">
                  {data.estado === 'Riego Activo' ? '💧' : 
                   data.estado === 'Emergencia' ? '🚨' : '🟢'}
                </div>
                <h2 className="text-2xl font-bold">{data.estado.toUpperCase()}</h2>
                <p className="text-sm opacity-90 mt-1">
                  {data.bombaActiva ? 'Bomba Activa' : 'Bomba Apagada'}
                </p>
              </div>
            </div>

            {/* Sensores */}
            <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
              <h3 className="text-lg font-bold text-gray-800 mb-4 flex items-center gap-2">
                <Activity size={20} className="text-purple-500" />
                Sensores en Tiempo Real
              </h3>
              
              <div className="space-y-4">
                {/* Temperatura */}
                <div className="bg-gradient-to-r from-orange-50 to-red-50 rounded-xl p-4">
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center gap-2">
                      <Thermometer size={20} className="text-orange-500" />
                      <span className="text-sm font-medium text-gray-700">Temperatura</span>
                    </div>
                    <span className="text-2xl font-bold text-orange-600">
                      {data.temperatura.toFixed(1)}°C
                    </span>
                  </div>
                </div>

                {/* Humedad Aire */}
                <div className="bg-gradient-to-r from-blue-50 to-cyan-50 rounded-xl p-4">
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center gap-2">
                      <Wind size={20} className="text-blue-500" />
                      <span className="text-sm font-medium text-gray-700">Humedad Aire</span>
                    </div>
                    <span className="text-2xl font-bold text-blue-600">
                      {data.humedadAire.toFixed(0)}%
                    </span>
                  </div>
                </div>

                {/* Humedad Suelo */}
                <div className="bg-gradient-to-r from-green-50 to-emerald-50 rounded-xl p-4">
                  <div className="flex items-center justify-between mb-3">
                    <div className="flex items-center gap-2">
                      <Sprout size={20} className="text-green-500" />
                      <span className="text-sm font-medium text-gray-700">Humedad Suelo</span>
                    </div>
                    <span className="text-2xl font-bold text-green-600">
                      {data.humedadSuelo}%
                    </span>
                  </div>
                  <div className="w-full bg-gray-200 rounded-full h-3 overflow-hidden">
                    <div
                      className="h-full transition-all duration-500 rounded-full"
                      style={{
                        width: `${data.humedadSuelo}%`,
                        backgroundColor: getHumidityColor(data.humedadSuelo)
                      }}
                    />
                  </div>
                </div>

                {/* Flujo */}
                <div className="bg-gradient-to-r from-cyan-50 to-blue-50 rounded-xl p-4">
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center gap-2">
                      <Droplet size={20} className="text-cyan-500" />
                      <span className="text-sm font-medium text-gray-700">Flujo de Agua</span>
                    </div>
                    <span className="text-2xl font-bold text-cyan-600">
                      {data.flujo.toFixed(2)} L/min
                    </span>
                  </div>
                </div>
              </div>
            </div>

            {/* Estadísticas */}
            <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
              <h3 className="text-lg font-bold text-gray-800 mb-4 flex items-center gap-2">
                <BarChart3 size={20} className="text-indigo-500" />
                Estadísticas
              </h3>
              
              <div className="grid grid-cols-2 gap-4">
                <div className="bg-purple-50 rounded-xl p-4 text-center">
                  <div className="text-3xl font-bold text-purple-600">
                    {data.ciclosRiego}
                  </div>
                  <div className="text-xs text-gray-600 mt-1">Ciclos de Riego</div>
                </div>
                
                <div className="bg-blue-50 rounded-xl p-4 text-center">
                  <div className="text-3xl font-bold text-blue-600">
                    {data.aguaTotal.toFixed(1)}L
                  </div>
                  <div className="text-xs text-gray-600 mt-1">Agua Total</div>
                </div>
                
                <div className="bg-green-50 rounded-xl p-4 text-center">
                  <div className="text-3xl font-bold text-green-600">
                    {data.eficiencia.toFixed(0)}%
                  </div>
                  <div className="text-xs text-gray-600 mt-1">Eficiencia</div>
                </div>
                
                <div className="bg-amber-50 rounded-xl p-4 text-center">
                  <div className="text-2xl font-bold text-amber-600">
                    {data.tiempoOptimo || 30}s
                  </div>
                  <div className="text-xs text-gray-600 mt-1">Tiempo Óptimo</div>
                </div>
                
                <div className="bg-indigo-50 rounded-xl p-4 text-center">
                  <div className="text-2xl font-bold text-indigo-600">
                    {data.tipoSuelo}
                  </div>
                  <div className="text-xs text-gray-600 mt-1">Tipo de Suelo</div>
                </div>
              </div>
            </div>

            {/* Selector de Cultivo */}
            <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
              <h3 className="text-lg font-bold text-gray-800 mb-4">
                🌾 Cultivo Actual: {data.cultivoNombre}
              </h3>
              
              <div className="grid grid-cols-2 gap-2">
                {cultivos.map((cultivo) => (
                  <button
                    key={cultivo.id}
                    onClick={() => changeCultivo(cultivo.id)}
                    className={`p-3 rounded-xl font-medium transition ${
                      cultivo.id === data.cultivoId
                        ? 'bg-gradient-to-r from-green-500 to-emerald-500 text-white shadow-lg'
                        : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
                    }`}
                  >
                    {cultivo.nombre}
                  </button>
                ))}
              </div>
            </div>

            {/* Información de Riego */}
            {data.bombaActiva && (
              <div className="bg-gradient-to-r from-blue-500 to-cyan-500 rounded-3xl shadow-2xl p-6 mb-4 text-white">
                <h3 className="text-lg font-bold mb-3 flex items-center gap-2">
                  <Droplet size={20} />
                  Riego en Progreso
                </h3>
                <div className="space-y-2">
                  <div className="flex justify-between">
                    <span className="text-sm opacity-90">Tiempo transcurrido:</span>
                    <span className="font-bold">{data.tiempoRiegoTranscurrido || 0}s / {data.tiempoMaxRiego || 300}s</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-sm opacity-90">Flujo actual:</span>
                    <span className="font-bold">{data.flujo?.toFixed(2) || '0.00'} L/min</span>
                  </div>
                  <div className="w-full bg-white/20 rounded-full h-2 mt-2">
                    <div
                      className="bg-white rounded-full h-2 transition-all"
                      style={{
                        width: `${Math.min((data.tiempoRiegoTranscurrido / (data.tiempoMaxRiego || 300)) * 100, 100)}%`
                      }}
                    />
                  </div>
                </div>
              </div>
            )}

            {/* Alertas */}
            {(data.emergenciaActiva || data.bloqueoBomba || data.fuga || data.modoSeguro || !data.dhtOperativo) && (
              <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
                <h3 className="text-lg font-bold text-gray-800 mb-4 flex items-center gap-2">
                  <AlertTriangle size={20} className="text-red-500" />
                  Estado del Sistema
                </h3>
                
                <div className="space-y-2">
                  {data.emergenciaActiva && (
                    <div className="bg-red-50 border-l-4 border-red-500 p-3 rounded">
                      <p className="text-sm font-semibold text-red-700">🚨 EMERGENCIA ACTIVA</p>
                      <p className="text-xs text-red-600 mt-1">{data.razonEmergencia || 'Razón desconocida'}</p>
                      <p className="text-xs text-red-500 mt-1">El sistema se recuperará automáticamente en 60 segundos</p>
                    </div>
                  )}
                  {data.bloqueoBomba && (
                    <div className="bg-red-50 border-l-4 border-red-500 p-3 rounded">
                      <p className="text-sm text-red-700">⚠️ Bloqueo detectado en bomba</p>
                      <p className="text-xs text-red-600 mt-1">Verifique que la bomba esté funcionando correctamente</p>
                    </div>
                  )}
                  {data.fuga && (
                    <div className="bg-red-50 border-l-4 border-red-500 p-3 rounded">
                      <p className="text-sm text-red-700">🚨 Fuga de agua detectada</p>
                      <p className="text-xs text-red-600 mt-1">Se detectó flujo de agua cuando la bomba está apagada</p>
                    </div>
                  )}
                  {data.modoSeguro && (
                    <div className="bg-yellow-50 border-l-4 border-yellow-500 p-3 rounded">
                      <p className="text-sm text-yellow-700">⚠️ Sistema en modo seguro</p>
                      <p className="text-xs text-yellow-600 mt-1">Algunos sensores no responden. Usando valores de respaldo.</p>
                    </div>
                  )}
                  {!data.dhtOperativo && (
                    <div className="bg-yellow-50 border-l-4 border-yellow-500 p-3 rounded">
                      <p className="text-sm text-yellow-700">⚠️ Sensor DHT en modo respaldo</p>
                      <p className="text-xs text-yellow-600 mt-1">Usando últimos valores válidos. Intente reparar el sensor.</p>
                    </div>
                  )}
                </div>
              </div>
            )}

            {/* Configuración Manual */}
            <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
              <h3 className="text-lg font-bold text-gray-800 mb-4 flex items-center gap-2">
                <Settings size={20} className="text-indigo-500" />
                Configuración Manual
              </h3>
              
              <ConfiguracionManual 
                data={data} 
                ipAddress={ipAddress}
                onConfigChange={async () => {
                  await fetchData();
                  // Forzar una segunda actualización después de un breve delay para asegurar valores actualizados
                  setTimeout(() => fetchData(), 500);
                }}
                agregarNotificacion={agregarNotificacion}
                normalizeUrl={normalizeUrl}
              />
            </div>

            {/* Control Manual */}
            <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
              <h3 className="text-lg font-bold text-gray-800 mb-4">🎮 Control Manual</h3>
              
              <div className="grid grid-cols-2 gap-3">
                <button
                  onClick={() => sendCommand('regar')}
                  disabled={comandoEjecutando || data?.bombaActiva || data?.emergenciaActiva}
                  className={`flex items-center justify-center gap-2 bg-gradient-to-r from-blue-500 to-cyan-500 text-white py-4 rounded-xl font-semibold hover:shadow-lg transition ${
                    comandoEjecutando || data?.bombaActiva || data?.emergenciaActiva ? 'opacity-50 cursor-not-allowed' : ''
                  }`}
                >
                  {comandoEjecutando === 'regar' ? (
                    <RefreshCw size={20} className="animate-spin" />
                  ) : (
                    <Play size={20} />
                  )}
                  {data?.bombaActiva ? 'Riego Activo' : 'Iniciar'}
                </button>
                
                <button
                  onClick={() => sendCommand('detener')}
                  disabled={comandoEjecutando || !data?.bombaActiva}
                  className={`flex items-center justify-center gap-2 bg-gradient-to-r from-gray-500 to-gray-600 text-white py-4 rounded-xl font-semibold hover:shadow-lg transition ${
                    comandoEjecutando || !data?.bombaActiva ? 'opacity-50 cursor-not-allowed' : ''
                  }`}
                >
                  {comandoEjecutando === 'detener' ? (
                    <RefreshCw size={20} className="animate-spin" />
                  ) : (
                    <Square size={20} />
                  )}
                  Detener
                </button>
              </div>
              
              <div className="grid grid-cols-2 gap-3 mt-3">
                <button
                  onClick={() => {
                    if (window.confirm('¿Está seguro de activar el modo emergencia? El sistema se detendrá completamente.')) {
                      sendCommand('emergency');
                    }
                  }}
                  disabled={comandoEjecutando}
                  className={`flex items-center justify-center gap-2 bg-gradient-to-r from-red-500 to-pink-500 text-white py-4 rounded-xl font-semibold hover:shadow-lg transition ${
                    comandoEjecutando ? 'opacity-50 cursor-not-allowed' : ''
                  }`}
                >
                  {comandoEjecutando === 'emergency' ? (
                    <RefreshCw size={20} className="animate-spin" />
                  ) : (
                    <AlertTriangle size={20} />
                  )}
                  Emergencia
                </button>
                
                <button
                  onClick={() => sendCommand('reset')}
                  disabled={comandoEjecutando}
                  className={`flex items-center justify-center gap-2 bg-gradient-to-r from-indigo-500 to-purple-500 text-white py-4 rounded-xl font-semibold hover:shadow-lg transition ${
                    comandoEjecutando ? 'opacity-50 cursor-not-allowed' : ''
                  }`}
                >
                  {comandoEjecutando === 'reset' ? (
                    <RefreshCw size={20} className="animate-spin" />
                  ) : (
                    <RefreshCw size={20} />
                  )}
                  Reiniciar
                </button>
              </div>
            </div>

            {/* ThingSpeak */}
            {data.channelID > 0 && (
              <div className="bg-white rounded-3xl shadow-2xl p-6 mb-4">
                <a
                  href={`https://thingspeak.com/channels/${data.channelID}`}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="flex items-center justify-center gap-2 bg-gradient-to-r from-cyan-500 to-blue-500 text-white py-4 rounded-xl font-semibold hover:shadow-lg transition"
                >
                  <BarChart3 size={20} />
                  Ver Historial en ThingSpeak
                </a>
              </div>
            )}

            {/* Footer */}
            <div className="bg-white rounded-3xl shadow-2xl p-4 text-center">
              <p className="text-xs text-gray-500">
                Última actualización: {lastUpdate.toLocaleTimeString()}
              </p>
              <p className="text-xs text-gray-400 mt-1">
                Sistema de Riego Autonómico v4.0
              </p>
            </div>
          </>
        )}
      </div>
    </div>
  );
}