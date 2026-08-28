/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        command: {
          bg: '#090D16',
          card: '#0F172A',
          cardHover: '#1E293B',
          border: '#1E293B',
          borderLight: '#334155',
          text: '#F8FAFC',
          muted: '#94A3B8',
          subtle: '#64748B'
        },
        risk: {
          normal: '#10B981',    // Emerald
          normalBg: 'rgba(16, 185, 129, 0.12)',
          warning: '#F59E0B',   // Amber
          warningBg: 'rgba(245, 158, 11, 0.15)',
          critical: '#EF4444',  // Crimson Red
          criticalBg: 'rgba(239, 68, 68, 0.18)',
          offline: '#64748B',   // Slate
          offlineBg: 'rgba(100, 116, 139, 0.12)'
        },
        hazard: {
          fire: '#FF5722',
          flood: '#0284C7',
          pollution: '#8B5CF6'
        }
      },
      fontFamily: {
        sans: ['Inter', 'system-ui', '-apple-system', 'BlinkMacSystemFont', 'Segoe UI', 'Roboto', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'Consolas', 'monospace']
      },
      animation: {
        'pulse-fast': 'pulse 1.2s cubic-bezier(0.4, 0, 0.6, 1) infinite',
        'ping-slow': 'ping 2s cubic-bezier(0, 0, 0.2, 1) infinite',
      }
    },
  },
  plugins: [],
}
