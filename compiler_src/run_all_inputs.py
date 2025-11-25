import os
import subprocess
import shutil

# --- CONFIGURACIÓN ---
# Si "token.cpp" no existe en tu carpeta, bórralo de esta lista.
programa = ["main.cpp", "scanner.cpp", "token.cpp", "parser.cpp", "ast.cpp", "visitor.cpp"]

INPUT_DIR = "inputs"
OUTPUT_DIR = "outputs"

# --- 1. COMPILACIÓN DEL COMPILADOR ---
print("🔨 Compilando proyecto C++...")
compile_cmd = ["g++"] + programa + ["-o", "a.out"]
result = subprocess.run(compile_cmd, capture_output=True, text=True)

if result.returncode != 0:
    print("❌ Error en compilación C++:\n", result.stderr)
    exit(1)

print("✅ Compilación exitosa.\n")

os.makedirs(OUTPUT_DIR, exist_ok=True)
# Aseguramos que inputs exista, aunque sea para leer
if not os.path.exists(INPUT_DIR):
    print(f"⚠️ La carpeta {INPUT_DIR} no existe. Creándola...")
    os.makedirs(INPUT_DIR)

# --- 2. FUNCIÓN PARA GENERAR IMAGEN (Con Filtro de Seguridad) ---
def generar_imagen_memoria(filepath, filename):
    print(f"🎨 Generando diagrama de memoria para {filename}...")
    
    # Ejecutamos con --debug para obtener el código DOT
    res = subprocess.run(["./a.out", filepath, "--debug"], capture_output=True, text=True)
    
    output_texto = res.stdout
    
    # --- FILTRO DE SEGURIDAD ---
    # Buscamos dónde empieza el grafo para ignorar mensajes previos como "Parser exitoso"
    inicio_grafo = output_texto.find("digraph MemoryFlow")
    
    if inicio_grafo != -1:
        # Nos quedamos solo con la parte del grafo
        contenido_dot = output_texto[inicio_grafo:]
        
        # Guardamos el archivo .dot limpio
        if not contenido_dot.endswith("}"):
            print("   🔧 Reparando DOT (faltaba llave de cierre)...")
            contenido_dot += "\n}"

        dot_path = os.path.join(OUTPUT_DIR, filename.replace(".txt", ".dot"))
        with open(dot_path, "w") as f:
            f.write(contenido_dot)
            
        # Generamos la imagen PNG
        png_path = os.path.join(OUTPUT_DIR, filename.replace(".txt", ".png"))
        try:
            subprocess.run(["dot", "-Tpng", dot_path, "-o", png_path], check=True)
            print(f"   ✅ Imagen guardada: {png_path}")
        except FileNotFoundError:
            print("   ⚠️  Error: 'dot' no encontrado. Instala Graphviz.")
        except Exception as e:
            print(f"   ⚠️  Error generando PNG: {e}")
    else:
        print("   ⚠️  No se encontró código de grafo en la salida (¿Falló el debug?).")

# --- 3. BUCLE PRINCIPAL ---
for i in range(1, 8):
    filename = f"input{i}.txt"
    filepath = os.path.join(INPUT_DIR, filename)

    if not os.path.isfile(filepath):
        # Opcional: intentar buscar en la raíz si no está en inputs/
        if os.path.isfile(filename):
            filepath = filename
        else:
            continue

    print(f"\n🔹 PROCESANDO: {filename}")

    # A) Generar Imagen Visual (Timeline)
    generar_imagen_memoria(filepath, filename)    
    
    # B) Generar ASM y Ejecutar
    # (Eliminé la línea redundante que tenías aquí)
    run_cmd = ["./a.out", filepath]
    result = subprocess.run(run_cmd, capture_output=True, text=True)

    # Imprimir logs del compilador C++ (útil para ver si hubo errores de semántica)
    if result.stdout.strip(): print(f"   [C++ STDOUT]: {result.stdout.strip()}")
    if result.stderr.strip(): print(f"   [C++ STDERR]: {result.stderr.strip()}")

    # Verificar si se creó el .s
    # Dependiendo de tu main.cpp, puede generarse en inputs/ o en la raíz
    posibles_s = [
        os.path.join(INPUT_DIR, filename.replace(".txt", ".s")), # inputs/input1.s
        filename.replace(".txt", ".s")                           # input1.s
    ]
    
    tokens_file = None
    for p in posibles_s:
        if os.path.isfile(p):
            tokens_file = p
            break

    if tokens_file:
        # Mover a outputs/
        dest_asm = os.path.join(OUTPUT_DIR, f"input_{i}.s")
        if os.path.exists(dest_asm): os.remove(dest_asm)
        shutil.move(tokens_file, dest_asm)

        # Compilar el .s con GCC
        exec_path = os.path.join(OUTPUT_DIR, f"input_{i}.exec")
        compile_s_cmd = ["g++", dest_asm, "-o", exec_path]
        compile_s = subprocess.run(compile_s_cmd, capture_output=True, text=True)

        # Guardar reporte de ejecución
        run_output_file = os.path.join(OUTPUT_DIR, f"input_{i}_run.txt")
        with open(run_output_file, "w", encoding="utf-8") as fout:
            fout.write(f"=== Compilación ASM de {filename} ===\n")
            if compile_s.returncode != 0:
                fout.write("FALLÓ LA COMPILACIÓN CON GCC\n")
                fout.write(compile_s.stderr)
                print(f"   ❌ Error compilando ASM: {compile_s.stderr.strip()}")
            else:
                # Ejecutar el binario final
                print(f"   🚀 Ejecutando binario...")
                run_exec = subprocess.run([exec_path], capture_output=True, text=True)
                
                fout.write("STDOUT:\n" + run_exec.stdout + "\n")
                fout.write("STDERR:\n" + run_exec.stderr + "\n")
                print(f"   [Output Final]: {run_exec.stdout.strip()}")
        
        print(f"   📄 Reporte guardado en {run_output_file}")

    else:
        print(f"   ❌ No se generó archivo .s para {filename}")