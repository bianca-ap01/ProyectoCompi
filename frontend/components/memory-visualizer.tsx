"use client"

import { useEffect, useMemo, useState } from "react"
import { ImageIcon } from "lucide-react"
import type { StackFrame } from "@/lib/compiler-service"

interface MemoryVisualizerProps {
  imageBase64?: string
  stack?: StackFrame[]
  output?: string
  activeIndex: number
  setActiveIndex: (idx: number) => void
}

export function MemoryVisualizer({ imageBase64, stack, output, activeIndex, setActiveIndex }: MemoryVisualizerProps) {
  const hasStack = useMemo(() => !!stack && stack.length > 0, [stack])
  const snaps = stack ?? []

  const diffForIndex = (idx: number) => {
    const snap = snaps[idx]
    const prevSnap = idx > 0 ? snaps[idx - 1] : undefined
    if (!snap) return { added: [] as string[], updated: new Set<string>() }
    const prevMap = prevSnap ? new Map(prevSnap.vars.map((v) => [v.name, v])) : new Map<string, typeof snap.vars[number]>()
    const added: string[] = []
    const updated = new Set<string>()
    snap.vars.forEach((v) => {
      const p = prevMap.get(v.name)
      if (!p) {
        added.push(v.name)
      } else if (p.value !== v.value || p.offset !== v.offset) {
        updated.add(v.name)
      }
    })
    return { added, updated }
  }

  const current = snaps[activeIndex] ?? snaps[0]
  const prev = activeIndex > 0 ? snaps[activeIndex - 1] : undefined

  useEffect(() => {
    if (activeIndex >= snaps.length) setActiveIndex(0)
  }, [snaps.length, activeIndex, setActiveIndex])

  const changes = useMemo(() => {
    if (!current) return { added: [] as string[], updated: new Set<string>() }
    const prevMap = prev ? new Map(prev.vars.map((v) => [v.name, v])) : new Map<string, typeof current.vars[number]>()
    const added: string[] = []
    const updated = new Set<string>()
    current.vars.forEach((v) => {
      const p = prevMap.get(v.name)
      if (!p) {
        added.push(v.name)
      } else if (p.value !== v.value || p.offset !== v.offset) {
        updated.add(v.name)
      }
    })
    return { added, updated }
  }, [current, prev])

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

  if (hasStack) {
    return (
      <div className="h-full w-full flex flex-col bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 border border-white/10">
        {/* Fila 1: Snapshots */}
        <div className="border-b border-white/10 bg-white/5 px-3 py-2 space-y-2">
          <div className="flex items-center justify-between text-xs text-slate-300">
            <span className="uppercase tracking-[0.15em]">Código / Snapshots</span>
            <span>{snaps.length} paso(s)</span>
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-2 max-h-40 overflow-auto scrollbar-transparent">
            {snaps.map((s, idx) => {
              const diff = diffForIndex(idx)
              return (
                <button
                  key={`${s.label}-${idx}`}
                  onClick={() => setActiveIndex(idx)}
                  className={`text-left rounded-xl border border-white/10 px-3 py-2 transition-colors ${
                    activeIndex === idx
                      ? "bg-sky-500/80 text-slate-900 shadow-sm shadow-sky-500/40"
                      : "bg-white/10 text-slate-100 hover:bg-white/20"
                  }`}
                >
                  <div className="flex items-center justify-between text-[11px] font-semibold font-mono uppercase tracking-wide">
                    <span className="truncate">
                      {s.line && s.line > 0 ? `L${s.line} · ${s.label}` : s.label}
                    </span>
                  </div>
                  {(diff.added.length > 0 || diff.updated.size > 0) && (
                    <div className="text-[10px] text-slate-200 mt-1 space-y-1">
                      {diff.added.length > 0 && (
                        <div>
                          <span className="text-emerald-300 font-semibold">+ </span>
                          {diff.added.join(", ")}
                        </div>
                      )}
                      {diff.updated.size > 0 && (
                        <div>
                          <span className="text-amber-300 font-semibold">↻ </span>
                          {Array.from(diff.updated).join(", ")}
                        </div>
                      )}
                    </div>
                  )}
                </button>
              )
            })}
          </div>
        </div>

        {/* Fila 2: stack del snapshot activo */}
        <div className="flex-1 overflow-auto p-4">
          {current ? (
            <div className="rounded-2xl border border-white/10 bg-[#0d152a]/80 backdrop-blur-xl p-3 shadow-lg shadow-black/30 space-y-2">
              <div className="flex items-center justify-between text-xs text-slate-200 mb-1">
                <span className="font-semibold text-white uppercase tracking-wide text-[11px] flex items-center gap-2">
                  {current.label}
                  {current.line !== undefined && current.line > 0 && (
                    <span className="text-[10px] text-sky-200 font-mono px-2 py-0.5 rounded bg-sky-500/15 border border-sky-500/30">
                      L{current.line}
                    </span>
                  )}
                </span>
              </div>
              {(changes.added.length > 0 || changes.updated.size > 0) && (
                <div className="text-[11px] text-slate-200">
                  {changes.added.length > 0 && (
                    <div className="mb-1">
                      <span className="text-emerald-300 font-semibold">Nuevas:</span>{" "}
                      {changes.added.join(", ")}
                    </div>
                  )}
                  {changes.updated.size > 0 && (
                    <div>
                      <span className="text-amber-300 font-semibold">Actualizadas:</span>{" "}
                      {Array.from(changes.updated).join(", ")}
                    </div>
                  )}
                </div>
              )}
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
                    {current.vars.map((v, i) => (
                      <tr
                        key={`${v.name}-${i}`}
                        className={`text-slate-100 ${
                          changes.updated.has(v.name)
                            ? "bg-amber-500/30"
                            : "odd:bg-white/5 even:bg-white/10"
                        }`}
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
                    {current.vars.length === 0 && (
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
          ) : (
            <div className="text-sm text-slate-400">Sin snapshot seleccionado.</div>
          )}
        </div>
      </div>
    )
  }

  // Fallback: imagen base64 del backend
  return (
    <div className="h-full w-full flex flex-col bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 border border-white/10">
      <div className="flex items-center justify-between px-3 py-2 border-b border-white/10 bg-white/5 text-xs text-slate-200">
        <span className="uppercase tracking-[0.15em] text-slate-400">Memoria / Stack (imagen)</span>
      </div>

      <div className="flex-1 overflow-auto p-4">
        <div className="relative w-full min-h-[300px] flex justify-center">
          <img
            src={`data:image/png;base64,${imageBase64}`}
            alt="Diagrama de Memoria"
            className="shadow-2xl shadow-black/40 border border-white/10 rounded-2xl max-w-full h-auto"
          />
        </div>
      </div>
    </div>
  )
}
