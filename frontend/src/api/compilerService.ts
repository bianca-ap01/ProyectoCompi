import axios from 'axios';
import type { CompilationResponse } from '../types/index.ts';

const API_URL = 'http://localhost:8000'; // Tu backend

export const compileCode = async (code: string): Promise<CompilationResponse> => {
    try {
        const response = await axios.post<CompilationResponse>(`${API_URL}/compile`, {
            code: code
        });
        return response.data;
    } catch (error) {
        console.error("Error conectando con el compilador:", error);
        throw error;
    }
};