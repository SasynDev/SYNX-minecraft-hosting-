import React, { useState, useEffect, useRef } from 'react';
import { Send, Terminal as TerminalIcon, RotateCcw, AlertCircle } from 'lucide-react';
import { api } from '../api';

interface ConsoleViewProps {
  serverId: string;
}

export const ConsoleView: React.FC<ConsoleViewProps> = ({ serverId }) => {
  const [lines, setLines] = useState<string[]>([]);
  const [command, setCommand] = useState('');
  const [loading, setLoading] = useState(false);
  const bottomRef = useRef<HTMLDivElement>(null);

  // Poll or stream console lines
  useEffect(() => {
    let active = true;

    const fetchLogs = async () => {
      try {
        const res = await api.getConsoleLines(serverId);
        if (active) {
          setLines(res.lines || []);
        }
      } catch (err) {
        // silent fallback
      }
    };

    fetchLogs();
    const interval = setInterval(fetchLogs, 1500);

    return () => {
      active = false;
      clearInterval(interval);
    };
  }, [serverId]);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [lines]);

  const handleSendCommand = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!command.trim()) return;

    const cmd = command;
    setCommand('');
    setLoading(true);

    try {
      await api.sendCommand(serverId, cmd);
      const res = await api.getConsoleLines(serverId);
      setLines(res.lines || []);
    } catch (err) {
      // Show error in console
      setLines((prev) => [...prev, `[SYNX ERROR] Failed to send command: ${err}`]);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="bg-synx-card border border-synx-border rounded-2xl flex flex-col h-[550px] overflow-hidden shadow-xl">
      {/* Console Top Toolbar */}
      <div className="bg-synx-dark/80 px-4 py-3 border-b border-synx-border flex items-center justify-between">
        <div className="flex items-center gap-2">
          <TerminalIcon className="w-4 h-4 text-synx-purpleLight" />
          <span className="text-xs font-bold text-white tracking-wide uppercase">Live Server Console</span>
        </div>
        <div className="flex items-center gap-2">
          <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
          <span className="text-[11px] text-synx-muted font-mono">Stream Active</span>
        </div>
      </div>

      {/* Terminal Screen */}
      <div className="flex-1 bg-[#090814] p-4 font-['JetBrains_Mono',monospace] text-xs text-slate-300 overflow-y-auto space-y-1">
        {lines.length === 0 ? (
          <div className="text-synx-muted/60 italic py-8 text-center">
            Server console output will appear here once the instance is started...
          </div>
        ) : (
          lines.map((line, idx) => {
            let color = 'text-slate-300';
            if (line.includes('[ERROR]') || line.includes('crashed')) color = 'text-rose-400 font-semibold';
            else if (line.includes('[WARN]')) color = 'text-amber-300';
            else if (line.includes('[INFO]')) color = 'text-slate-300';
            else if (line.startsWith('>')) color = 'text-synx-purpleLight font-bold';

            return (
              <div key={idx} className={`leading-relaxed break-words ${color}`}>
                {line}
              </div>
            );
          })
        )}
        <div ref={bottomRef} />
      </div>

      {/* Command Input Bar */}
      <form onSubmit={handleSendCommand} className="p-3 bg-synx-dark/90 border-t border-synx-border flex items-center gap-2">
        <div className="flex items-center gap-2 flex-1 bg-synx-card border border-synx-border rounded-xl px-3 py-1.5 focus-within:border-synx-purple transition-colors">
          <span className="text-synx-purpleLight font-mono text-sm font-bold">&gt;</span>
          <input
            type="text"
            value={command}
            onChange={(e) => setCommand(e.target.value)}
            placeholder="Type a server command (e.g. say Hello, op player, gamemode creative)..."
            className="w-full bg-transparent text-sm text-white focus:outline-none font-mono"
          />
        </div>
        <button
          type="submit"
          disabled={loading || !command.trim()}
          className="px-4 py-2 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white text-xs font-bold shadow-glow transition-all flex items-center gap-1.5 disabled:opacity-40"
        >
          <Send className="w-3.5 h-3.5" />
          <span>Execute</span>
        </button>
      </form>
    </div>
  );
};

