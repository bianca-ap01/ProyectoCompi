// frontend/tailwind.config.js

/** @type {import('tailwindcss').Config} */
export default {
  // MUY IMPORTANTE: Asegura que Tailwind escanee todos tus archivos TSX
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {},
  },
  plugins: [],
}