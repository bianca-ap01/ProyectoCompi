from pydantic import BaseModel
from typing import Optional, List


class StackVar(BaseModel):
    name: str
    value: str
    offset: Optional[int] = None
    type: Optional[str] = None


class StackFrame(BaseModel):
    label: str                      # p.e. nombre de función o bloque
    vars: List[StackVar] = []       # variables visibles en ese frame
    sp: Optional[int] = None        # opcional: dirección/offset del stack pointer


class SourceCode(BaseModel):
    code: str


class CompilationResponse(BaseModel):
    success: bool
    output: str                        # Salida del programa (printf)
    image_b64: Optional[str] = None    # Imagen en base64 para el <img> del frontend
    logs: str                          # Logs de compilación (errores gcc, etc)
    stack: List[StackFrame] = []       # Representación estructurada del stack/memoria
    asm: Optional[str] = None          # Código ensamblador generado (.s)
