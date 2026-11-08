/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        synx: {
          dark: '#080711',
          bg: '#0e0c1b',
          card: '#161329',
          cardHover: '#1f1b3b',
          purple: '#7c3aed',
          purpleGlow: '#9333ea',
          purpleLight: '#a855f7',
          border: '#272248',
          text: '#f3f4f6',
          muted: '#9ca3af',
          success: '#10b981',
          danger: '#ef4444',
          warn: '#f59e0b'
        }
      },
      boxShadow: {
        'glow': '0 0 25px -5px rgba(124, 58, 237, 0.4)',
        'glow-sm': '0 0 15px -3px rgba(124, 58, 237, 0.3)',
      }
    },
  },
  plugins: [],
}

