// frontend/postcss.config.js

export default {
  plugins: {
    '@tailwindcss/postcss': {}, // <--- SOLUCIÓN: Usar el paquete correcto
    autoprefixer: {},
  },
}
