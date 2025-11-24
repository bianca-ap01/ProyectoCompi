import base64
import subprocess
import os
from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI()

class CodeRequest(BaseModel):
    code: str

@app.post("/compile")
def compile_and_visualize(req: CodeRequest):
    # 1. Guardar código C en archivo temporal
    with open("input.c", "w") as f:
        f.write(req.code)

    # 2. Ejecutar C++ para obtener el DOT (stdout)
    # Suponiendo que tu ejecutable se llama 'compiler'
    # ./compiler input.c --graph
    proc = subprocess.run(
        ["./compiler", "input.c", "--graph"], 
        capture_output=True, 
        text=True
    )

    if proc.returncode != 0:
        return {"error": proc.stderr}

    dot_content = proc.stdout

    # 3. Guardar el DOT y generar la imagen con Graphviz
    dot_filename = "ast.dot"
    png_filename = "ast.png"
    
    with open(dot_filename, "w") as f:
        f.write(dot_content)

    # Llamada al sistema 'dot' (Graphviz)
    # dot -Tpng ast.dot -o ast.png
    try:
        subprocess.run(["dot", "-Tpng", dot_filename, "-o", png_filename], check=True)
    except FileNotFoundError:
        return {"error": "Graphviz no está instalado en el servidor (comando 'dot' no encontrado)."}

    # 4. Leer la imagen y convertir a Base64
    with open(png_filename, "rb") as image_file:
        encoded_string = base64.b64encode(image_file.read()).decode('utf-8')

    # Retornar JSON listo para usar en <img src="...">
    return {
        "status": "success",
        "image_base64": f"data:image/png;base64,{encoded_string}",
        "raw_dot": dot_content # Opcional, por si quieres debugear
    }