"use client"

import { useEffect, useRef, useState } from "react"
import Editor, { type OnMount } from "@monaco-editor/react"
import type * as monaco from "monaco-editor"

interface GrammarPanelProps {
  code: string
  setCode: (value: string) => void
  activeLine?: number
}

export function GrammarPanel({ code, setCode, activeLine }: GrammarPanelProps) {
  const [editorInstance, setEditorInstance] = useState<monaco.editor.IStandaloneCodeEditor | null>(null)
  const monacoRef = useRef<typeof import("monaco-editor")>()
  const decorationsRef = useRef<string[]>([])

  const handleEditorChange = (value: string | undefined) => {
    if (value !== undefined) setCode(value)
  }

  const handleMount: OnMount = (editor, monacoInstance) => {
    setEditorInstance(editor)
    monacoRef.current = monacoInstance
  }

  useEffect(() => {
    if (!editorInstance || !monacoRef.current) return
    const m = monacoRef.current
    const decos =
      activeLine && activeLine > 0
        ? [
            {
              range: new m.Range(activeLine, 1, activeLine, 1),
              options: {
                isWholeLine: true,
                className: "code-active-line",
                marginClassName: "code-active-line-margin",
              },
            },
          ]
        : []
    decorationsRef.current = editorInstance.deltaDecorations(decorationsRef.current, decos)
    if (activeLine && activeLine > 0) {
      editorInstance.revealLineInCenter(activeLine)
    }
  }, [activeLine, editorInstance])

  return (
    <div className="h-full w-full" id="code-editor-pane">
      <Editor
        height="100%"
        defaultLanguage="c"
        theme="vs-dark"
        value={code}
        onChange={handleEditorChange}
        onMount={handleMount}
        options={{
          minimap: { enabled: false },
          fontSize: 13,
          fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
          lineNumbers: "on",
          scrollBeyondLastLine: false,
          automaticLayout: true,
          tabSize: 4,
          padding: { top: 12, bottom: 12 },
          renderLineHighlight: "gutter",
          cursorBlinking: "smooth",
          smoothScrolling: true,
          bracketPairColorization: { enabled: true },
        }}
      />
    </div>
  )
}
