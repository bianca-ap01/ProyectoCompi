export interface CompilationResponse {
    success: boolean;
    output: string;
    image_b64: string;
    logs: string;
}

export interface SourceCode {
    code: string;
}