import { useState } from 'react';
import { Play, Terminal, Activity, AlertCircle } from 'lucide-react';
import { GrammarPanel } from './components/GrammarPanel';
import { MemoryVisualizer } from './components/MemoryVisualizer';
import { compileCode } from './api/compilerService';

function App() {
  // Código inicial de ejemplo
  const [code, setCode] = useState(`#include <stdio.h>\n\nint main() {\n    int x = 10;\n    printf("Hola desde el Frontend! x = %d", x);\n    return 0;\n}`);
  
  const [output, setOutput] = useState("");
  const [logs, setLogs] = useState("");
  const [image, setImage] = useState("");
  const [loading, setLoading] = useState(false);
  const [activeTab, setActiveTab] = useState<'output' | 'memory' | 'logs'>('output');

  const handleRun = async () => {
    setLoading(true);
    setOutput("");
    setLogs("");
    setImage(""); // Limpiar imagen anterior
    
    try {
      const result = await compileCode(code);
      setOutput(result.output);
      setLogs(result.logs);
      setImage(result.image_b64);
      
      // Si hay imagen, cambiar automáticamente a la pestaña de memoria
      if (result.image_b64) {
        setActiveTab('memory');
      } else {
        setActiveTab('output');
      }
      
    } catch (error) {
      setOutput("Error de conexión con el servidor.");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="h-screen w-screen flex flex-col bg-[#1e1e1e] text-white overflow-hidden">
      
      {/* --- HEADER --- */}
      <header className="h-14 bg-[#333333] flex items-center px-4 justify-between border-b border-black">
        <h1 className="font-bold text-lg text-blue-400">Compiler<span className="text-white">Pro</span></h1>
        
        <button 
          onClick={handleRun}
          disabled={loading}
          className={`flex items-center gap-2 px-6 py-2 rounded font-semibold transition-all ${
            loading 
              ? 'bg-gray-600 cursor-not-allowed text-gray-400' 
              : 'bg-green-600 hover:bg-green-700 text-white'
          }`}
        >
          {loading ? 'Compilando...' : <><Play size={18} fill="white" /> EJECUTAR</>}
        </button>
      </header>

      {/* --- MAIN CONTENT (Grid Split) --- */}
      <div className="flex-1 flex overflow-hidden">
        
        {/* IZQUIERDA: EDITOR */}
        <div className="w-1/2 h-full">
          <GrammarPanel code={code} setCode={setCode} />
        </div>

        {/* DERECHA: PANELES DE RESULTADO */}
        <div className="w-1/2 h-full flex flex-col bg-[#1e1e1e] border-l border-gray-700">
          
          {/* Tabs de navegación */}
          <div className="flex bg-[#252526] border-b border-gray-700">
            <button 
              onClick={() => setActiveTab('output')}
              className={`flex items-center gap-2 px-4 py-3 text-sm border-t-2 ${activeTab === 'output' ? 'border-blue-500 bg-[#1e1e1e] text-white' : 'border-transparent text-gray-400 hover:bg-[#2d2d2d]'}`}
            >
              <Terminal size={16} /> Consola
            </button>
            <button 
              onClick={() => setActiveTab('memory')}
              className={`flex items-center gap-2 px-4 py-3 text-sm border-t-2 ${activeTab === 'memory' ? 'border-blue-500 bg-[#1e1e1e] text-white' : 'border-transparent text-gray-400 hover:bg-[#2d2d2d]'}`}
            >
              <Activity size={16} /> Memoria / Stack
            </button>
            <button 
              onClick={() => setActiveTab('logs')}
              className={`flex items-center gap-2 px-4 py-3 text-sm border-t-2 ${activeTab === 'logs' ? 'border-blue-500 bg-[#1e1e1e] text-white' : 'border-transparent text-gray-400 hover:bg-[#2d2d2d]'}`}
            >
              <AlertCircle size={16} /> Logs Compilador
            </button>
          </div>

          {/* Contenido de la derecha */}
          <div className="flex-1 overflow-auto bg-[#1e1e1e] relative">
            
            {activeTab === 'output' && (
              <div className="p-4 font-mono text-sm whitespace-pre-wrap text-gray-300">
                {output || <span className="text-gray-600 italic">Esperando salida...</span>}
              </div>
            )}

            {activeTab === 'memory' && (
               <MemoryVisualizer imageBase64={image} />
            )}

            {activeTab === 'logs' && (
              <div className="p-4 font-mono text-xs whitespace-pre-wrap text-yellow-500">
                {logs || <span className="text-gray-600 italic">No hay logs disponibles.</span>}
              </div>
            )}

          </div>
        </div>
      </div>
    </div>
  );
}

export default App;