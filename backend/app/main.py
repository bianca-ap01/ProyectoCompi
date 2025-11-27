from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from .models import SourceCode, CompilationResponse
from .compiler_service import CompilerService

app = FastAPI(title="C Compiler Backend")

# Configurar CORS 
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Instancia global del servicio
service = None

@app.on_event("startup")
def startup_event():
    global service
    try:
        service = CompilerService() # Esto compila el C++ al arrancar
    except Exception as e:
        print(f"Error fatal iniciando compilador: {e}")

@app.post("/compile", response_model=CompilationResponse)
def compile_code(source: SourceCode):
    if not service:
        raise HTTPException(status_code=500, detail="Compiler service not initialized")
    
    result = service.process_code(source.code)
    return result

@app.get("/")
def read_root():
    return {"message": "API del Compilador funcionando 🚀"}