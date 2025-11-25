interface MemoryVisualizerProps {
    imageBase64: string;
}

export const MemoryVisualizer = ({ imageBase64 }: MemoryVisualizerProps) => {
    if (!imageBase64) {
        return (
            <div className="h-full flex items-center justify-center text-gray-500 text-sm italic">
                La visualización de memoria aparecerá aquí...
            </div>
        );
    }

    return (
        <div className="h-full w-full overflow-auto bg-white p-4 flex justify-center items-start">
            <img 
                src={`data:image/png;base64,${imageBase64}`} 
                alt="Diagrama de Memoria" 
                className="max-w-none shadow-lg border border-gray-200"
            />
        </div>
    );
};