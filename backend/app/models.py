from pydantic import BaseModel
from typing import Optional

class SourceCode(BaseModel):
    code: str

class CompilationResponse(BaseModel):
    success: bool
    output: str           # Salida del programa (printf)
    image_b64: str        # Imagen en base64 para el <img> del frontend
    logs: str             # Logs de compilación (errores gcc, etc)