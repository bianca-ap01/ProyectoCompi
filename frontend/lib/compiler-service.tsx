export interface CompilationResponse {
  success: boolean
  output: string
  image_b64?: string
  logs: string
  stack?: StackFrame[]
  asm?: string
}

export type StackVar = {
  name: string
  value: string
  offset?: number
  type?: string
}

export type StackFrame = {
  label: string
  vars: StackVar[]
  sp?: number
  line?: number
}

const API_URL = process.env.NEXT_PUBLIC_COMPILER_API_URL || "http://localhost:8000"

export async function compileCode(code: string): Promise<CompilationResponse> {
  try {
    const response = await fetch(`${API_URL}/compile`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ code }),
    })

    if (!response.ok) {
      throw new Error("Compilation failed")
    }

    return response.json()
  } catch (error) {
    console.error("Error conectando con el compilador:", error)
    throw error
  }
}
