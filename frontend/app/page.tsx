"use client"

import { useState } from "react"
import { Play, Activity, AlertCircle, Code2, Zap, FileCode2 } from "lucide-react"
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
  const [activeTab, setActiveTab] = useState<"memory" | "logs">("memory")
  const [activeSnap, setActiveSnap] = useState(0)
  const activeSnapshot = stack[activeSnap]
  const totalSnapshots = stack.length

  const goPrev = () => setActiveSnap((idx) => (idx > 0 ? idx - 1 : idx))
  const goNext = () => setActiveSnap((idx) => (idx + 1 < totalSnapshots ? idx + 1 : idx))
  const handleRun = async () => {
    setLoading(true)
    setOutput("")
    setLogs("")
    setImage("")
    setStack([])
    setAsm("")
    setActiveSnap(0)

    try {
      const result = await compileCode(code)
      setOutput(result.output)
      setLogs(result.logs)
      setImage(result.image_b64 || "")
      setStack(result.stack || [])
      setAsm(result.asm || "")
      setActiveTab(result.stack && result.stack.length > 0 ? "memory" : "logs")
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
        {/* Controles de paso a paso */}
        <div className="flex items-center justify-between mb-3 text-sm text-muted-foreground">
          <div className="flex items-center gap-3">
            <span className="font-medium text-foreground">Modo línea por línea</span>
            <span className="text-xs">
              {totalSnapshots > 0
                ? `Paso ${activeSnap + 1} de ${totalSnapshots}`
                : "Sin pasos aún"}
            </span>
          </div>
          <div className="flex items-center gap-2">
            <button
              onClick={goPrev}
              disabled={activeSnap === 0 || totalSnapshots === 0}
              className="px-3 py-1.5 rounded-md border border-border bg-muted/40 text-foreground text-xs disabled:opacity-50"
            >
              ← Anterior
            </button>
            <button
              onClick={goNext}
              disabled={activeSnap + 1 >= totalSnapshots || totalSnapshots === 0}
              className="px-3 py-1.5 rounded-md border border-border bg-muted/40 text-foreground text-xs disabled:opacity-50"
            >
              Siguiente →
            </button>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-4 h-[calc(100vh-7rem)]">
          {/* Code Editor Panel */}
          <div className="lg:col-span-1 rounded-xl bg-card border border-border overflow-hidden flex flex-col">
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
            <div className="flex-1 min-h-0 flex flex-col">
              <div className="flex-1 min-h-0">
                <GrammarPanel code={code} setCode={setCode} />
              </div>
              <div className="border-t border-border bg-muted/20 h-32 px-3 py-2 overflow-auto scrollbar-transparent">
                <div className="text-xs uppercase tracking-[0.15em] text-muted-foreground mb-1">Salida</div>
                <pre className="text-xs font-mono text-foreground whitespace-pre-wrap">
                  {output || <span className="text-muted-foreground italic">Sin salida.</span>}
                </pre>
              </div>
            </div>
          </div>

          {/* Columna ASM */}
          <div className="rounded-xl bg-card border border-border overflow-hidden flex flex-col">
            <div className="px-4 h-11 border-b border-border bg-muted/30 flex items-center justify-between">
              <div className="flex items-center gap-2 text-xs text-muted-foreground">
                <FileCode2 size={12} />
                <span className="uppercase tracking-[0.15em]">ASM</span>
              </div>
              {activeSnapshot?.line ? (
                <div className="text-[11px] text-muted-foreground font-mono">
                  Línea {activeSnapshot.line}
                </div>
              ) : null}
            </div>
            <div className="flex-1 p-4 overflow-auto min-h-[480px] scrollbar-transparent">
              <pre className="font-mono text-xs text-accent-foreground whitespace-pre-wrap leading-relaxed bg-muted/20 rounded-md p-3 border border-border h-full">
                {(() => {
                  const lines = (asm || "").split("\n")
                  // Usa la posición ordinal de cada SNAP en el ASM (n-th SNAP ↔ n-th snapshot)
                  const allMarkers = lines
                    .map((l, idx) => ({ l, idx }))
                    .filter(({ l }) => l.trim().startsWith("# SNAP"))
                    .map(({ idx }) => idx)

                  const markerIndex = activeSnap + 1 < allMarkers.length ? activeSnap + 1 : allMarkers.length - 1
                  const markerLineIdx = markerIndex >= 0 ? allMarkers[markerIndex] : -1
                  const endIdx = markerLineIdx >= 0 ? markerLineIdx + 1 : lines.length
                  const view = lines.slice(0, endIdx) // acumulado hasta el paso actual
                  return view.map((line, i) => {
                    const highlight = markerLineIdx >= 0 && i + (0 /* offset slice */) === markerLineIdx
                    return (
                      <div
                        key={i}
                        className={highlight ? "bg-sky-500/20 px-1 rounded text-slate-900" : undefined}
                      >
                        {line || " "}
                      </div>
                    )
                  })
                })()}
              </pre>
            </div>
          </div>

          {/* Columna Memoria / Logs */}
          <div className="rounded-xl bg-card border border-border overflow-hidden flex flex-col">
            <div className="flex items-center gap-1 px-2 h-11 border-b border-border bg-muted/30">
              {(["memory", "logs"] as const).map((tab) => (
                <button
                  key={tab}
                  onClick={() => setActiveTab(tab)}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded-md text-xs font-medium transition-all duration-200 ${
                    activeTab === tab
                      ? "bg-accent text-accent-foreground shadow-sm"
                      : "text-muted-foreground hover:text-foreground hover:bg-muted"
                  }`}
                >
                  {tab === "memory" && <Activity size={12} />}
                  {tab === "logs" && <AlertCircle size={12} />}
                  <span className="capitalize">{tab === "memory" ? "Memoria" : "Logs"}</span>
                </button>
              ))}
            </div>
            <div className="flex-1 overflow-hidden">
              {activeTab === "memory" && (
                <div className="h-full">
                  <MemoryVisualizer
                    imageBase64={image}
                    stack={stack}
                    output={output}
                    activeIndex={activeSnap}
                    setActiveIndex={setActiveSnap}
                  />
                </div>
              )}
              {activeTab === "logs" && (
                <div className="h-full p-4 overflow-auto">
                  <pre className="font-mono text-xs text-warning whitespace-pre-wrap leading-relaxed">
                    {logs || <span className="text-muted-foreground italic">No hay logs disponibles.</span>}
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
