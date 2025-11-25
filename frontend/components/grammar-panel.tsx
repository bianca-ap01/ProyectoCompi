"use client"

import Editor from "@monaco-editor/react"

interface GrammarPanelProps {
  code: string
  setCode: (value: string) => void
}

export function GrammarPanel({ code, setCode }: GrammarPanelProps) {
  const handleEditorChange = (value: string | undefined) => {
    if (value) setCode(value)
  }

  return (
    <div className="h-full w-full">
      <Editor
        height="100%"
        defaultLanguage="c"
        theme="vs-dark"
        value={code}
        onChange={handleEditorChange}
        options={{
          minimap: { enabled: false },
          fontSize: 14,
          fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
          lineNumbers: "on",
          scrollBeyondLastLine: false,
          automaticLayout: true,
          tabSize: 4,
          padding: { top: 16, bottom: 16 },
          renderLineHighlight: "gutter",
          cursorBlinking: "smooth",
          smoothScrolling: true,
          bracketPairColorization: { enabled: true },
        }}
      />
    </div>
  )
}
