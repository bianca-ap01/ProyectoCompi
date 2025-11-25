import os
import subprocess
import base64
import uuid

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
        # Asegurar que existe temp
        os.makedirs(TEMP_DIR, exist_ok=True)
        self.compile_cpp_compiler()

    def compile_cpp_compiler(self):
        """Compila tu código C++ una sola vez al iniciar el servidor"""
        print("🔨 Compilando el compilador C++...")
        
        # Lista de archivos fuente
        sources = [
            os.path.join(COMPILER_DIR, f) 
            for f in ["main.cpp", "scanner.cpp", "parser.cpp", "ast.cpp", "visitor.cpp", "token.cpp"]
            # Agrega token.cpp si lo usas
        ]
        
        cmd = ["g++"] + sources + ["-o", COMPILER_BIN]
        res = subprocess.run(cmd, capture_output=True, text=True)
        
        if res.returncode != 0:
            raise RuntimeError(f"Error compilando el backend C++:\n{res.stderr}")
        print("✅ Compilador C++ listo.")

    def process_code(self, source_code: str):
        # Usamos un ID único para que si 2 usuarios usan la web a la vez no se mezclen
        unique_id = str(uuid.uuid4())[:8]
        input_path = os.path.join(TEMP_DIR, f"{unique_id}.txt")
        dot_path = os.path.join(TEMP_DIR, f"{unique_id}.dot")
        png_path = os.path.join(TEMP_DIR, f"{unique_id}.png")
        asm_path = os.path.join(TEMP_DIR, f"{unique_id}.s")
        exec_path = os.path.join(TEMP_DIR, f"{unique_id}.exe")

        logs = []
        output_text = ""
        image_b64 = ""
        success = True

        try:
            # 1. Guardar el código fuente del usuario
            with open(input_path, "w") as f:
                f.write(source_code)

            # -------------------------------------------------
            # PASO A: Generar Imagen (Debug Mode)
            # -------------------------------------------------
            res_debug = subprocess.run([COMPILER_BIN, input_path, "--debug"], capture_output=True, text=True)
            
            # Extraer DOT y corregir llave si falta (tu fix anterior)
            raw_out = res_debug.stdout
            start_idx = raw_out.find("digraph MemoryFlow")
            
            if start_idx != -1:
                dot_content = raw_out[start_idx:].strip()
                if not dot_content.endswith("}"):
                    dot_content += "\n}"
                
                with open(dot_path, "w") as f:
                    f.write(dot_content)
                
                # Convertir a PNG
                subprocess.run(f"dot -Tpng {dot_path} -o {png_path}", shell=True, check=True)
                
                # Leer PNG y pasar a Base64
                if os.path.exists(png_path):
                    with open(png_path, "rb") as img_file:
                        b64_bytes = base64.b64encode(img_file.read())
                        image_b64 = b64_bytes.decode('utf-8')
            else:
                logs.append("⚠️ No se pudo generar la visualización (No graph found).")

            # -------------------------------------------------
            # PASO B: Ejecutar Código (Assembly Mode)
            # -------------------------------------------------
            # Ejecutar tu compilador para sacar el .s
            # NOTA: Tu main.cpp genera el .s en inputs/ o local. 
            # Como estamos ejecutando desde temp, hay que tener cuidado.
            # Lo ideal es mover tu lógica de main.cpp para que acepte output path, 
            # pero por ahora asumiremos que lo deja al lado del input o en inputs/
            
            # Para asegurar compatibilidad con tu main.cpp actual, creamos carpeta inputs/ ficticia si es necesario
            # O simplemente corremos el binario:
            res_asm = subprocess.run([COMPILER_BIN, input_path], capture_output=True, text=True)
            
            # Buscar donde demonios quedó el .s (Tu main.cpp lo deja en inputs/nombre.s o ./nombre.s)
            # Vamos a buscarlo:
            possible_asm = input_path.replace(".txt", ".s")
            
            # Si tu main.cpp fuerza la carpeta "inputs/", habrá que moverlo, 
            # pero asumamos que lo genera relativo al input.
            
            if os.path.exists(possible_asm):
                # Compilar con GCC
                res_gcc = subprocess.run(["g++", possible_asm, "-o", exec_path], capture_output=True, text=True)
                if res_gcc.returncode == 0:
                    # Ejecutar el binario final
                    res_exec = subprocess.run([exec_path], capture_output=True, text=True)
                    output_text = res_exec.stdout + res_exec.stderr
                else:
                    logs.append("Error GCC: " + res_gcc.stderr)
                    success = False
            else:
                # Intento de fallback: a veces tu main lo guarda en inputs/ID.s
                logs.append("No se generó archivo Assembly (.s).")
                logs.append("Stdout del compilador: " + res_asm.stdout)
                logs.append("Stderr del compilador: " + res_asm.stderr)
                success = False

        except Exception as e:
            logs.append(f"Error interno del servidor: {str(e)}")
            success = False
        finally:
            # Limpieza de archivos temporales (Opcional, para no llenar el disco)
            # os.remove(input_path) ...etc
            pass

        return {
            "success": success,
            "output": output_text,
            "image_b64": image_b64, # Frontend usa: <img src="data:image/png;base64,..." />
            "logs": "\n".join(logs)
        }