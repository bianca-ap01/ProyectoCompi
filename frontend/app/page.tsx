"use client"

import { useEffect, useRef, useState } from "react"
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
  const [asmByLine, setAsmByLine] = useState<Record<number, string[]> | undefined>(undefined)
  const [showFullAsm, setShowFullAsm] = useState(false)
  const asmActiveRef = useRef<HTMLDivElement | null>(null)
  const [loading, setLoading] = useState(false)
  const [activeTab, setActiveTab] = useState<"memory" | "logs">("memory")
  const [activeSnap, setActiveSnap] = useState(0)
  const activeSnapshot = stack[activeSnap]
  const totalSnapshots = stack.length
  const CODE_KEY = "orbit-code"

  useEffect(() => {
    if (typeof window === "undefined") return
    const saved = localStorage.getItem(CODE_KEY)
    if (saved && saved.length > 0) {
      setCode(saved)
    }
  }, [])

  const handleCodeChange = (value: string) => {
    setCode(value)
    if (typeof window !== "undefined") {
      localStorage.setItem(CODE_KEY, value)
    }
  }

  const goPrev = () => setActiveSnap((idx) => (idx > 0 ? idx - 1 : idx))
  const goNext = () => setActiveSnap((idx) => (idx + 1 < totalSnapshots ? idx + 1 : idx))
  const handleRun = async () => {
    setLoading(true)
    setOutput("")
    setLogs("")
    setImage("")
    setStack([])
    setAsm("")
    setAsmByLine(undefined)
    setShowFullAsm(false)
    setActiveSnap(0)

    try {
      const result = await compileCode(code)
      setOutput(result.output)
      setLogs(result.logs)
      setImage(result.image_b64 || "")
      setStack(result.stack || [])
    setAsm(result.asm || "")
    setAsmByLine(result.asm_by_line)
    setActiveTab(result.stack && result.stack.length > 0 ? "memory" : "logs")
  } catch (error) {
      setOutput("Error de conexión con el servidor.")
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    if (asmActiveRef.current && !showFullAsm) {
      asmActiveRef.current.scrollIntoView({ behavior: "smooth", block: "center" })
    }
  }, [activeSnap, showFullAsm, asmByLine])

  const rawLine = stack[activeSnap]?.line ?? -1
  const editorLine = rawLine > 0 ? rawLine : undefined

  useEffect(() => {
    const editorPane = document.getElementById("code-editor-pane")
    if (editorPane && editorLine) {
      const lines = editorPane.querySelectorAll(".cm-line")
      const target = lines[editorLine - 1] as HTMLElement | undefined
      if (target) {
        target.scrollIntoView({ behavior: "smooth", block: "center" })
      }
    }
  }, [activeSnap, stack, editorLine])

  return (
    <div className="min-h-screen bg-background text-foreground text-sm">
      {/* Header */}
      <header className="border-b border-border bg-card/50 backdrop-blur-sm sticky top-0 z-50">
        <div className="max-w-[1600px] mx-auto px-4 h-12 flex items-center justify-between">
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
            <div className="hidden sm:flex items-center gap-2 px-3 py-1.5 rounded-full bg-muted border border-border text-[11px]">
              <span className={`w-1.5 h-1.5 rounded-full ${loading ? "bg-warning animate-pulse" : "bg-success"}`} />
              <span className="text-muted-foreground">{loading ? "Compilando..." : "Listo"}</span>
            </div>

            <button
              onClick={handleRun}
              disabled={loading}
              className={`flex items-center gap-2 px-3 py-2 rounded-lg text-xs font-medium transition-all duration-200 ${
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
      <main className="max-w-[1600px] mx-auto p-3 md:p-4">
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-3 min-h-[calc(100vh-7rem)]">
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
                <GrammarPanel code={code} setCode={handleCodeChange} activeLine={editorLine} />
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
              <div className="flex items-center gap-2">
                <button
                  onClick={goPrev}
                  disabled={activeSnap === 0 || totalSnapshots === 0}
                  className="px-2 py-1 rounded border border-border bg-muted/40 text-foreground text-[11px] disabled:opacity-50"
                >
                  ←
                </button>
                <button
                  onClick={goNext}
                  disabled={activeSnap + 1 >= totalSnapshots || totalSnapshots === 0}
                  className="px-2 py-1 rounded border border-border bg-muted/40 text-foreground text-[11px] disabled:opacity-50"
                >
                  →
                </button>
                {activeSnapshot?.line ? (
                  <div className="text-[11px] text-muted-foreground font-mono">
                    Línea {activeSnapshot.line}
                  </div>
                ) : null}
                <button
                  onClick={() => {
                    const next = !showFullAsm
                    setShowFullAsm(next)
                    if (!next && stack.length > 0) {
                      setActiveSnap(stack.length - 1)
                    }
                    if (next) {
                      // al ver código completo, desplazarse al inicio
                      const container = document.getElementById("asm-scroll-container")
                      if (container) container.scrollTop = 0
                    }
                  }}
                  className="text-[11px] px-2 py-1 rounded border border-border hover:bg-muted/40 text-foreground"
                >
                  {showFullAsm ? "Ver por línea" : "Ver código completo"}
                </button>
              </div>
            </div>
            <div
              id="asm-scroll-container"
              className="flex-1 p-4 overflow-y-auto max-h-[calc(100vh-14rem)] scrollbar-transparent"
            >
              <div className="space-y-3">
                {(() => {
                  if (asmByLine && !showFullAsm) {
                    const targetLine = stack[activeSnap]?.line
                    const prolog: string[] = []
                    const mainStartLine =
                      stack
                        .filter((s) => (s.func || "").toLowerCase() === "main" && (s.line ?? 0) > 0)
                        .map((s) => s.line ?? Number.MAX_SAFE_INTEGER)
                        .reduce((a, b) => Math.min(a, b), Number.MAX_SAFE_INTEGER)
                    if (asm) {
                      const linesAll = (asm || "").split("\n")
                      for (const l of linesAll) {
                        if (l.startsWith("# SNAPIDX")) break
                        if (l.trim().length > 0) prolog.push(l)
                      }
                    }

                    const labelByLine = (line: number, instrs: string[]) => {
                      const snap = stack.find((s) => s.line === line)
                      if (snap?.label) {
                        const lbl = snap.label.toLowerCase()
                        if (lbl.includes("start")) return ""
                        return snap.label
                      }
                      const joined = instrs.join(" ")
                      if (joined.includes("printf@PLT")) return "print"
                      if (joined.includes("call ")) return "call"
                      if (joined.includes("pushq %rbp") && joined.includes("movq %rsp, %rbp")) return "prolog"
                      if (joined.includes(".globl")) return "func"
                      return ""
                    }

                    let blocks = Object.entries(asmByLine)
                      .map(([lineStr, instrs]) => ({
                        line: parseInt(lineStr, 10),
                        instrs,
                        label: labelByLine(parseInt(lineStr, 10), instrs),
                      }))
                      .filter((b) => !Number.isNaN(b.line) && b.line >= -1)
                    if (prolog.length > 0) {
                      blocks.unshift({ line: -1, instrs: prolog, label: "prolog" })
                    }
                    blocks.sort((a, b) => a.line - b.line)
                    const firstPosLine = blocks.find((b) => b.line >= 1)?.line ?? Number.MAX_SAFE_INTEGER
                    const mainStart = mainStartLine === Number.MAX_SAFE_INTEGER ? firstPosLine : mainStartLine
                    const prologBlocksRaw = blocks.filter((b) => b.line < mainStart)
                    const prologBlocks =
                      prologBlocksRaw.length > 1 ? prologBlocksRaw.slice(1) : prologBlocksRaw // quitar el primer prolog, dejar el resto
                    const mainBlocks = blocks.filter((b) => b.line >= mainStart)

                    const mergedBlocks = [...prologBlocks, ...mainBlocks]
                    let activeLine = targetLine
                    if (activeLine !== undefined && activeLine <= 0) {
                      activeLine = prologBlocks[0]?.line ?? -1
                    } else if (activeLine === undefined && mergedBlocks.length > 0) {
                      activeLine = mergedBlocks[mergedBlocks.length - 1].line
                    }

                    const renderBlock = (blk: typeof blocks[number], idx: number) => (
                      <div
                        key={`${blk.line}-${idx}`}
                        className={`rounded-lg border border-border bg-muted/30 p-3 ${
                          activeLine === blk.line ? "ring-2 ring-accent/60" : ""
                        }`}
                        ref={activeLine === blk.line ? (el) => (asmActiveRef.current = el) : undefined}
                      >
                        <div className="flex items-center justify-between text-[11px] font-mono uppercase text-muted-foreground mb-2">
                          <span>{blk.line < 1 ? "Prolog" : `Línea ${blk.line}`}</span>
                          <span className="text-foreground font-semibold">{blk.label || ""}</span>
                        </div>
                        <pre className="font-mono text-xs text-accent-foreground whitespace-pre-wrap leading-relaxed">
                          {blk.instrs.length ? blk.instrs.join("\n") : "// sin instrucciones"}
                        </pre>
                      </div>
                    )

                    return <div className="space-y-2">{mergedBlocks.map((blk, idx) => renderBlock(blk, idx))}</div>
                  }

                  // Si no hay asm_by_line, mostrar ASM completo
                  return (
                    <div className="rounded-lg border border-border bg-muted/30 p-3">
                      <pre className="font-mono text-xs text-accent-foreground whitespace-pre-wrap leading-relaxed">
                        {(asm || "")
                          .split("\n")
                          .filter((l) => !l.trim().startsWith("# SNAPIDX"))
                          .join("\n") || "// sin instrucciones"}
                      </pre>
                    </div>
                  )
                })()}
              </div>
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
