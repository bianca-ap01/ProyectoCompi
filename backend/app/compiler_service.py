import os
import subprocess
import uuid
import json
from typing import List

# Rutas
BASE_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# 2. Definir la ubicación de las fuentes C++
# Ahora están en la subcarpeta 'compiler_src' de la raíz.
COMPILER_DIR = os.path.join(BASE_DIR, "compiler_src")

# 3. Definir dónde guardar el binario compilado
TEMP_DIR = os.path.join(BASE_DIR, "temp")
COMPILER_BIN = os.path.join(TEMP_DIR, "compiler.out")

class CompilerService:
    def __init__(self):
        # prepara la carpeta temporal y compila el binario del compilador c++
        os.makedirs(TEMP_DIR, exist_ok=True)
        self.compile_cpp_compiler()

    def compile_cpp_compiler(self):
        """compila el compilador c++ una sola vez al iniciar el servidor"""
        sources = [
            os.path.join(COMPILER_DIR, f)
            for f in ["main.cpp", "scanner.cpp", "parser.cpp", "ast.cpp", "visitor.cpp", "token.cpp", "TypeChecker.cpp"]
        ]
        def needs_recompile():
            if not os.path.exists(COMPILER_BIN):
                return True
            bin_mtime = os.path.getmtime(COMPILER_BIN)
            return any(os.path.getmtime(src) > bin_mtime for src in sources)

        if not needs_recompile():
            print("binario del compilador vigente, no se recompila.")
            return

        print("compilando el compilador c...")
        
        cmd = ["g++"] + sources + ["-o", COMPILER_BIN]
        res = subprocess.run(cmd, capture_output=True, text=True)
        
        if res.returncode != 0:
            raise RuntimeError(f"Error compilando el backend C++:\n{res.stderr}")
        print("compilador c++ listo.")

    def process_code(self, source_code: str):
        # usa un id unico para no mezclar ejecuciones concurrentes
        unique_id = str(uuid.uuid4())[:8]
        input_path = os.path.join(TEMP_DIR, f"{unique_id}.txt")
        #dot_path = os.path.join(TEMP_DIR, f"{unique_id}.dot")
        #png_path = os.path.join(TEMP_DIR, f"{unique_id}.png")
        asm_path = os.path.join(TEMP_DIR, f"{unique_id}.s")
        exec_path = os.path.join(TEMP_DIR, f"{unique_id}.exe")
        stack_path = os.path.join(TEMP_DIR, f"{unique_id}_stack.json")

        logs = []
        output_text = ""
        asm_text = ""
        success = True
        stack_frames: List[dict] = []  # Estructura para el front; si el compilador genera JSON se rellena más abajo
        asm_by_line = None

        try:
            # 1. Guardar el código fuente del usuario
            with open(input_path, "w") as f:
                f.write(source_code)

            # -------------------------------------------------
            # ejecutar el compilador c++ y generar el .s
            res_asm = subprocess.run([COMPILER_BIN, input_path], capture_output=True, text=True)
                
            possible_asm = input_path.replace(".txt", ".s")
            
            # Si tu main.cpp fuerza la carpeta "inputs/", habrá que moverlo, 
            # pero asumamos que lo genera relativo al input.
            
            if os.path.exists(possible_asm):
                try:
                    with open(possible_asm, "r") as asm_file:
                        asm_text = asm_file.read()
                except Exception as e:
                    logs.append(f"No se pudo leer ASM: {e}")

                res_gcc = subprocess.run(["g++", possible_asm, "-o", exec_path], capture_output=True, text=True)
                if res_gcc.returncode == 0:
                    res_exec = subprocess.run([exec_path], capture_output=True, text=True)
                    output_text = res_exec.stdout + res_exec.stderr
                else:
                    logs.append("Error GCC: " + res_gcc.stderr)
                    success = False
            else:
                logs.append("No se generó archivo Assembly (.s).")
                logs.append("Stdout del compilador: " + res_asm.stdout)
                logs.append("Stderr del compilador: " + res_asm.stderr)
                success = False

        except Exception as e:
            logs.append(f"Error interno del servidor: {str(e)}")
            success = False

        # Intentar leer stack JSON si el compilador lo generó
        if os.path.exists(stack_path):
            try:
                with open(stack_path, "r") as sf:
                    stack_frames = json.load(sf)
            except Exception as e:
                logs.append(f"No se pudo leer stack JSON: {e}")

        # mapa linea -> instrucciones asm generado por el visitor (si existe)
        asm_map_path = stack_path + ".asm.json"
        if os.path.exists(asm_map_path):
            try:
                with open(asm_map_path, "r") as am:
                    asm_by_line = json.load(am)
            except Exception as e:
                logs.append(f"No se pudo leer ASM map: {e}")
        elif asm_text:
            # fallback usando SNAPIDX
            asm_by_line = {}
            current_line = None
            for raw in asm_text.splitlines():
                line = raw.strip("\n")
                if line.startswith("# SNAPIDX"):
                    parts = line.split("line")
                    if len(parts) > 1:
                        try:
                            current_line = int(parts[-1].strip())
                        except ValueError:
                            current_line = None
                    else:
                        current_line = None
                    continue
                if current_line is not None:
                    asm_by_line.setdefault(current_line, []).append(line)

        return {
            "success": success,
            "output": output_text,
            "logs": "\n".join(logs),
            "stack": stack_frames,   # Datos estructurados para pintar el stack en el frontend
            "asm": asm_text,
            "asm_by_line": asm_by_line,
        }
