# Makefile para levantar backend y frontend con un solo comando

.PHONY: backend frontend dev

# Levanta el backend FastAPI (puerto 8000 por defecto)
backend:
	@cd backend && uvicorn app.main:app --reload

# Levanta el frontend (puerto 3000 por defecto)
frontend:
	@cd frontend && npm run dev

# Ejecuta backend y frontend en paralelo; Ctrl+C detiene ambos
dev:
	@echo "Iniciando backend (8000) y frontend (3000)..."
	@trap 'kill 0' INT TERM EXIT; \
	 (cd backend && uvicorn app.main:app --reload) & \
	 (cd frontend && npm run dev) & \
	 wait
