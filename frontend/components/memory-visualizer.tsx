"use client"

import { useMemo, useState } from "react"
import { ZoomIn, ZoomOut, RotateCcw, ImageIcon } from "lucide-react"
import type { StackFrame } from "@/lib/compiler-service"

interface MemoryVisualizerProps {
  imageBase64?: string
  stack?: StackFrame[]
}

export function MemoryVisualizer({ imageBase64, stack }: MemoryVisualizerProps) {
  const [zoom, setZoom] = useState(100)
  const clampZoom = (value: number) => Math.min(180, Math.max(40, value))

  const hasStack = useMemo(() => !!stack && stack.length > 0, [stack])
  const flattened = useMemo(() => {
    if (!stack) return []
    const items: Array<{frame: string; name: string; offset: number | undefined; type?: string; value?: string}> =
      []
    stack.forEach((fr) => {
      fr.vars.forEach((v) =>
        items.push({
          frame: fr.label,
          name: v.name,
          offset: v.offset,
          type: v.type,
          value: v.value,
        }),
      )
    })
    // Ordenamos de menor offset (más profundo) a mayor para simular el stack
    return items.sort((a, b) => (b.offset ?? 0) - (a.offset ?? 0))
  }, [stack])

  if (!hasStack && !imageBase64) {
    return (
      <div className="h-full flex flex-col items-center justify-center gap-3 text-muted-foreground p-6 bg-gradient-to-b from-slate-950 via-slate-900 to-slate-950">
        <div className="w-12 h-12 rounded-xl bg-white/5 border border-white/10 flex items-center justify-center">
          <ImageIcon className="opacity-70" size={20} />
        </div>
        <p className="text-sm text-center">La visualización de memoria aparecerá aquí...</p>
      </div>
    )
  }

  // Vista de stack estructurado
  if (hasStack) {
    return (
      <div className="h-full w-full flex flex-col bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 border border-white/10">
        <div className="flex items-center justify-between px-3 py-2 border-b border-white/10 bg-white/5 text-xs text-slate-200">
          <span className="uppercase tracking-[0.15em] text-slate-400">Memoria / Stack</span>
          <span className="text-slate-400">{stack?.length ?? 0} frame(s)</span>
        </div>

        <div className="flex-1 overflow-auto p-4 space-y-3">
          {flattened.length > 0 && (
            <div className="rounded-2xl border border-white/10 bg-white/5 p-3 shadow-inner shadow-black/20">
              <div className="text-xs text-slate-300 uppercase tracking-[0.12em] mb-2">Mapa del stack</div>
              <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-2">
                {flattened.map((item, idx) => (
                  <div
                    key={`${item.frame}-${item.name}-${idx}`}
                    className="rounded-xl bg-[#0f1b34] border border-white/10 px-3 py-2 text-[11px] font-mono text-slate-100"
                  >
                    <div className="flex justify-between text-[10px] text-slate-400 mb-1">
                      <span>{item.frame}</span>
                      <span>{item.offset !== undefined ? `${item.offset}(%rbp)` : ""}</span>
                    </div>
                    <div className="text-xs font-semibold">{item.name}</div>
                    <div className="text-[10px] text-slate-400">
                      {item.type || ""} {item.value ? `· ${item.value}` : ""}
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {stack?.map((frame, idx) => (
            <div
              key={`${frame.label}-${idx}`}
              className="rounded-2xl border border-white/10 bg-[#0d152a]/80 backdrop-blur-xl p-3 shadow-lg shadow-black/30"
            >
              <div className="flex items-center justify-between text-xs text-slate-200 mb-2">
                <span className="font-semibold text-white uppercase tracking-wide text-[11px]">
                  {frame.label}
                </span>
                {frame.sp !== undefined && <span className="text-slate-400">sp: {frame.sp}</span>}
              </div>
              <div className="w-full overflow-x-auto">
                <table className="min-w-full text-[11px] font-mono border border-white/10 rounded-xl overflow-hidden">
                  <thead className="bg-slate-800/70 text-slate-100 uppercase">
                    <tr>
                      <th className="px-2 py-2 border-r border-white/10 text-left">Var</th>
                      <th className="px-2 py-2 border-r border-white/10 text-left">Addr</th>
                      <th className="px-2 py-2 text-left">Val</th>
                    </tr>
                  </thead>
                  <tbody>
                    {frame.vars.map((v, i) => (
                      <tr
                        key={`${v.name}-${i}`}
                        className="odd:bg-white/5 even:bg-white/10 text-slate-100"
                      >
                        <td className="px-2 py-2 border-r border-white/10">
                          <div className="font-semibold">{v.name}</div>
                          <div className="text-[10px] text-slate-400">{v.type || ""}</div>
                        </td>
                        <td className="px-2 py-2 border-r border-white/10 text-sky-200">
                          {v.offset !== undefined ? `${v.offset}(%rbp)` : ""}
                        </td>
                        <td className="px-2 py-2 text-emerald-200">{v.value || "?"}</td>
                      </tr>
                    ))}
                    {frame.vars.length === 0 && (
                      <tr>
                        <td colSpan={3} className="px-2 py-3 text-center text-slate-400">
                          Sin variables en este frame
                        </td>
                      </tr>
                    )}
                  </tbody>
                </table>
              </div>
            </div>
          ))}
        </div>
      </div>
    )
  }

  // Fallback: imagen base64 del backend
  return (
    <div className="h-full w-full flex flex-col bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 border border-white/10">
      <div className="flex items-center justify-between px-3 py-2 border-b border-white/10 bg-white/5 text-xs text-slate-200">
        <span className="uppercase tracking-[0.15em] text-slate-400">Memoria / Stack (imagen)</span>
        <div className="flex items-center gap-2">
          <button
            onClick={() => setZoom((z) => clampZoom(z - 10))}
            className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 transition-colors border border-white/10"
            title="Alejar"
          >
            <ZoomOut size={14} />
          </button>
          <input
            type="range"
            min={40}
            max={180}
            value={zoom}
            onChange={(e) => setZoom(clampZoom(Number(e.target.value)))}
            className="w-24 accent-sky-400"
            aria-label="Zoom de memoria"
          />
          <button
            onClick={() => setZoom(100)}
            className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 transition-colors border border-white/10"
            title="Restablecer"
          >
            <RotateCcw size={14} />
          </button>
          <button
            onClick={() => setZoom((z) => clampZoom(z + 10))}
            className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 transition-colors border border-white/10"
            title="Acercar"
          >
            <ZoomIn size={14} />
          </button>
          <span className="text-slate-400 w-12 text-right">{zoom}%</span>
        </div>
      </div>

      <div className="flex-1 overflow-auto p-4">
        <div className="relative w-full min-h-[300px] flex justify-center">
          <img
            src={`data:image/png;base64,${imageBase64}`}
            alt="Diagrama de Memoria"
            className="shadow-2xl shadow-black/40 border border-white/10 rounded-2xl max-w-full h-auto"
            style={{
              transform: `scale(${zoom / 100})`,
              transformOrigin: "top center",
              transition: "transform 120ms ease-out",
            }}
          />
        </div>
      </div>
    </div>
  )
}
