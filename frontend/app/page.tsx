"use client"

import { useState } from "react"
import { Play, Terminal, Activity, AlertCircle, Code2, Zap, FileCode2 } from "lucide-react"
import { GrammarPanel } from "@/components/grammar-panel"
import { MemoryVisualizer } from "@/components/memory-visualizer"
import { compileCode, type StackFrame } from "@/lib/compiler-service"

export default function Home() {
  const [code, setCode] = useState(
    `#include <stdio.h>

int main() {
    int x;
    int y;
    long z;

    x = 1;
    y = 10;
    z = 1000000;

    x = 20;

    printf(" %d\\n", x);
    printf(" %d\\n", y);
    printf(" %ld\\n", z);

    return 0;
}`,
  )

  const [output, setOutput] = useState("")
  const [logs, setLogs] = useState("")
  const [image, setImage] = useState("")
  const [stack, setStack] = useState<StackFrame[]>([])
  const [asm, setAsm] = useState("")
  const [loading, setLoading] = useState(false)
  const [activeTab, setActiveTab] = useState<"output" | "memory" | "logs" | "asm">("output")

  const handleRun = async () => {
    setLoading(true)
    setOutput("")
    setLogs("")
    setImage("")
    setStack([])
    setAsm("")

    try {
      const result = await compileCode(code)
      setOutput(result.output)
      setLogs(result.logs)
      setImage(result.image_b64 || "")
      setStack(result.stack || [])
      setAsm(result.asm || "")

      if ((result.stack && result.stack.length > 0) || result.image_b64) {
        setActiveTab("memory")
      } else if (result.asm) {
        setActiveTab("asm")
      } else {
        setActiveTab("output")
      }
    } catch (error) {
      setOutput("Error de conexión con el servidor.")
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="min-h-screen bg-background text-foreground">
      {/* Header */}
      <header className="border-b border-border bg-card/50 backdrop-blur-sm sticky top-0 z-50">
        <div className="max-w-[1600px] mx-auto px-6 h-14 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="flex items-center justify-center w-8 h-8 rounded-lg bg-accent/10 border border-accent/20">
              <Code2 size={16} className="text-accent" />
            </div>
            <div className="flex items-center gap-2">
              <span className="font-semibold text-foreground">Orbit</span>
              <span className="text-muted-foreground text-sm hidden sm:inline">/ Compiler</span>
            </div>
          </div>

          <div className="flex items-center gap-4">
            <div className="hidden sm:flex items-center gap-2 px-3 py-1.5 rounded-full bg-muted border border-border text-xs">
              <span className={`w-1.5 h-1.5 rounded-full ${loading ? "bg-warning animate-pulse" : "bg-success"}`} />
              <span className="text-muted-foreground">{loading ? "Compilando..." : "Listo"}</span>
            </div>

            <button
              onClick={handleRun}
              disabled={loading}
              className={`flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-medium transition-all duration-200 ${
                loading
                  ? "bg-muted text-muted-foreground cursor-wait"
                  : "bg-accent hover:bg-accent/90 text-accent-foreground shadow-lg shadow-accent/20"
              }`}
            >
              {loading ? (
                <>
                  <div className="w-3.5 h-3.5 border-2 border-muted-foreground/30 border-t-muted-foreground rounded-full animate-spin" />
                  <span className="hidden sm:inline">Compilando</span>
                </>
              ) : (
                <>
                  <Play size={14} fill="currentColor" />
                  <span>Ejecutar</span>
                </>
              )}
            </button>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-[1600px] mx-auto p-4 md:p-6">
        <div className="grid grid-cols-1 lg:grid-cols-5 gap-4 h-[calc(100vh-7rem)]">
          {/* Code Editor Panel */}
          <div className="lg:col-span-3 rounded-xl bg-card border border-border overflow-hidden flex flex-col">
            <div className="flex items-center justify-between px-4 h-11 border-b border-border bg-muted/30">
              <div className="flex items-center gap-3">
                <div className="flex items-center gap-1.5">
                  <span className="w-3 h-3 rounded-full bg-destructive/80" />
                  <span className="w-3 h-3 rounded-full bg-warning/80" />
                  <span className="w-3 h-3 rounded-full bg-success/80" />
                </div>
                <div className="flex items-center gap-2 text-sm">
                  <span className="text-foreground font-medium">main.c</span>
                </div>
              </div>
              <div className="flex items-center gap-2 text-xs text-muted-foreground">
                <Zap size={12} />
                <span>x86-64</span>
              </div>
            </div>
            <div className="flex-1 min-h-0">
              <GrammarPanel code={code} setCode={setCode} />
            </div>
          </div>

          {/* Output Panel */}
          <div className="lg:col-span-2 rounded-xl bg-card border border-border overflow-hidden flex flex-col">
            {/* Tabs */}
            <div className="flex items-center gap-1 px-2 h-11 border-b border-border bg-muted/30">
              {(["output", "memory", "asm", "logs"] as const).map((tab) => (
                <button
                  key={tab}
                  onClick={() => setActiveTab(tab)}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md text-xs font-medium transition-all duration-200 ${
                    activeTab === tab
                      ? "bg-accent text-accent-foreground shadow-sm"
                      : "text-muted-foreground hover:text-foreground hover:bg-muted"
                  }`}
                >
                    {tab === "output" && <Terminal size={12} />}
                    {tab === "memory" && <Activity size={12} />}
                    {tab === "asm" && <FileCode2 size={12} />}
                    {tab === "logs" && <AlertCircle size={12} />}
                  <span className="capitalize">
                    {tab === "output"
                      ? "Consola"
                      : tab === "memory"
                        ? "Memoria"
                        : tab === "asm"
                          ? "ASM"
                          : "Logs"}
                  </span>
                </button>
              ))}
            </div>

            {/* Tab Content */}
            <div className="flex-1 overflow-hidden">
              {activeTab === "output" && (
                <div className="h-full p-4 overflow-auto">
                  <pre className="font-mono text-sm text-foreground/90 whitespace-pre-wrap leading-relaxed">
                    {output || <span className="text-muted-foreground italic">La salida aparecerá aquí...</span>}
                  </pre>
                </div>
              )}

              {activeTab === "memory" && (
                <div className="h-full">
                  <MemoryVisualizer imageBase64={image} stack={stack} />
                </div>
              )}

              {activeTab === "logs" && (
                <div className="h-full p-4 overflow-auto">
                  <pre className="font-mono text-xs text-warning whitespace-pre-wrap leading-relaxed">
                    {logs || <span className="text-muted-foreground italic">No hay logs disponibles.</span>}
                  </pre>
                </div>
              )}

              {activeTab === "asm" && (
                <div className="h-full p-4 overflow-auto">
                  <pre className="font-mono text-xs text-accent-foreground whitespace-pre-wrap leading-relaxed bg-muted/20 rounded-md p-3 border border-border">
                    {asm || <span className="text-muted-foreground italic">No hay ASM disponible.</span>}
                  </pre>
                </div>
              )}
            </div>
          </div>
        </div>
      </main>
    </div>
  )
}
