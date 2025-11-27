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
    line: Optional[int] = None      # línea del código fuente asociada al snapshot
    idx: Optional[int] = None       # índice global del snapshot
    func: Optional[str] = None      # nombre de la función a la que pertenece


class SourceCode(BaseModel):
    code: str


class CompilationResponse(BaseModel):
    success: bool
    output: str                        # Salida del programa (printf)
    logs: str                          # Logs de compilación (errores gcc, etc)
    stack: List[StackFrame] = []       # Representación estructurada del stack/memoria
    asm: Optional[str] = None          # Código ensamblador generado (.s)
    asm_by_line: Optional[dict] = None # Mapa línea fuente -> lista de instrucciones ASM
