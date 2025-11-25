// src/components/GrammarPanel.tsx

import Editor from '@monaco-editor/react';

interface GrammarPanelProps {
    code: string;
    setCode: (value: string) => void;
}

export const GrammarPanel = ({ code, setCode }: GrammarPanelProps) => {
    
    // Función de manejo del cambio de editor. Monaco asegura que 'value' es string o undefined.
    const handleEditorChange = (value: string | undefined) => {
        if (value) setCode(value);
    };

    return (
        <div className="h-full w-full flex flex-col bg-[#1e1e1e] border-r border-gray-700">
            <div className="bg-[#252526] text-gray-300 px-4 py-2 text-sm font-semibold border-b border-gray-700 flex justify-between items-center">
                <span>main.cpp</span>
                <span className="text-xs text-gray-500">C++ Compiler</span>
            </div>
            <div className="flex-1 pt-2">
                <Editor
                    height="100%"
                    defaultLanguage="cpp"
                    theme="vs-dark"
                    value={code}
                    onChange={handleEditorChange}
                    options={{
                        minimap: { enabled: false },
                        fontSize: 14,
                        lineNumbers: 'on',
                        scrollBeyondLastLine: false,
                        automaticLayout: true,
                        tabSize: 4
                    }}
                />
            </div>
        </div>
    );
};