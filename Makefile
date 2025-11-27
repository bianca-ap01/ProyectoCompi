# Makefile para levantar backend y frontend con un solo comando

.PHONY: backend frontend dev install-backend install-frontend install

# Levanta el backend FastAPI (puerto 8000 por defecto)
backend:
	@cd backend && if [ -d "../.venv" ]; then . ../.venv/bin/activate; fi; uvicorn app.main:app --reload

# Levanta el frontend (puerto 3000 por defecto)
frontend:
	@cd frontend && npm run dev

# Ejecuta backend y frontend en paralelo; Ctrl+C detiene ambos
dev:
	@echo "Iniciando backend (8000) y frontend (3000)..."
	 (cd backend && if [ -d "../.venv" ]; then . ../.venv/bin/activate; fi; uvicorn app.main:app --reload) & \
	 (cd frontend && npm run dev) & \
	 wait

# Instala dependencias de backend (usa .venv si existe)
install-backend:
	@cd backend && \
	if [ -d "../.venv" ]; then \
	  echo "Usando venv existente (.venv)"; \
	else \
	  echo "Creando venv en ../.venv"; \
	  python3 -m venv ../.venv; \
	fi; \
	. ../.venv/bin/activate && pip install -r requirements.txt

# Instala dependencias de frontend
install-frontend:
	@cd frontend && npm install

# Instala todo
install: install-backend install-frontend
	@echo "Instalación completa."

# Comportamiento por defecto: instalar y arrancar dev
all: install dev

# Hacer que `make` sin objetivo ejecute all
default: all

.DEFAULT_GOAL := default