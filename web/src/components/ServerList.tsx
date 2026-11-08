import React, { useState } from 'react';
import { Play, Square, RotateCw, Copy, Check, Terminal, ExternalLink, Users, ShieldAlert } from 'lucide-react';
import { InstanceSummary } from '../types';

interface ServerListProps {
  instances: InstanceSummary[];
  onSelectServer: (id: string) => void;
  onPowerAction: (id: string, action: 'start' | 'stop' | 'restart') => void;
  onCreateNew: () => void;
}

export const ServerList: React.FC<ServerListProps> = ({
  instances,
  onSelectServer,
  onPowerAction,
  onCreateNew,
}) => {
  const [copiedId, setCopiedId] = useState<string | null>(null);

  const copyToClipboard = (text: string, id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    navigator.clipboard.writeText(text);
    setCopiedId(id);
    setTimeout(() => setCopiedId(null), 2000);
  };

  if (instances.length === 0) {
    return (
      <div className="text-center py-20 px-4 bg-synx-card/40 rounded-2xl border border-synx-border max-w-2xl mx-auto my-12">
        <div className="w-16 h-16 rounded-2xl bg-synx-purple/10 border border-synx-purple/20 flex items-center justify-center mx-auto mb-4 text-synx-purpleLight">
          <Terminal className="w-8 h-8" />
        </div>
        <h3 className="text-xl font-bold text-white mb-2">No Minecraft Servers Running</h3>
        <p className="text-synx-muted text-sm max-w-md mx-auto mb-6">
          Deploy your first Minecraft instance on your Localhost machine. Paper, Purpur, Bedrock Crossplay, and Cracked accounts are supported out of the box.
        </p>
        <button
          onClick={onCreateNew}
          className="px-5 py-2.5 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white font-semibold text-sm shadow-glow transition-all"
        >
          Create New Server
        </button>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-2xl font-black text-white tracking-tight">Active Minecraft Instances</h2>
          <p className="text-sm text-synx-muted">Manage your orchestrated game servers running on Windows 11 host</p>
        </div>
        <button
          onClick={onCreateNew}
          className="px-4 py-2 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white text-sm font-semibold shadow-glow transition-all flex items-center gap-2"
        >
          <span>Deploy Server</span>
        </button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
        {instances.map((server) => {
          const isRunning = server.status === 'running';
          const isStopped = server.status === 'stopped';
          const isStarting = server.status === 'starting';

          const connectionAddress = `127.0.0.1:${server.port}`;

          return (
            <div
              key={server.id}
              onClick={() => onSelectServer(server.id)}
              className="group bg-synx-card hover:bg-synx-cardHover border border-synx-border hover:border-synx-purple/50 rounded-2xl p-5 transition-all cursor-pointer shadow-lg hover:shadow-glow-sm relative overflow-hidden flex flex-col justify-between"
            >
              {/* Top Accent Gradient Bar */}
              <div
                className={`absolute top-0 left-0 right-0 h-1 ${
                  isRunning
                    ? 'bg-emerald-500'
                    : isStarting
                    ? 'bg-amber-500 animate-pulse'
                    : 'bg-rose-500/50'
                }`}
              />

              <div>
                {/* Header: Name + Status Badge */}
                <div className="flex items-start justify-between gap-3 mb-3">
                  <div>
                    <h3 className="font-bold text-lg text-white group-hover:text-synx-purpleLight transition-colors">
                      {server.name}
                    </h3>
                    <p className="text-xs text-synx-muted font-mono">{server.identifier}</p>
                  </div>

                  <span
                    className={`text-xs px-2.5 py-1 rounded-full font-bold uppercase tracking-wider flex items-center gap-1.5 ${
                      isRunning
                        ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                        : isStarting
                        ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30 animate-pulse'
                        : 'bg-rose-500/10 text-rose-400 border border-rose-500/30'
                    }`}
                  >
                    <span
                      className={`w-1.5 h-1.5 rounded-full ${
                        isRunning ? 'bg-emerald-400' : isStarting ? 'bg-amber-400' : 'bg-rose-400'
                      }`}
                    />
                    {server.status}
                  </span>
                </div>

                {/* Badges: Server Type + Cracked Indicator */}
                <div className="flex flex-wrap items-center gap-2 mb-4">
                  <span className="text-xs px-2.5 py-0.5 rounded-md bg-synx-purple/10 text-synx-purpleLight border border-synx-purple/20 font-medium capitalize">
                    {server.server_type} {server.mc_version}
                  </span>

                  {server.cracked_enabled && (
                    <span className="text-xs px-2.5 py-0.5 rounded-md bg-emerald-500/10 text-emerald-400 border border-emerald-500/20 font-medium flex items-center gap-1">
                      <span>✓ Cracked Allowed</span>
                    </span>
                  )}

                  <span className="text-xs px-2.5 py-0.5 rounded-md bg-white/5 text-synx-muted border border-white/10 font-mono">
                    {server.ram_limit_mb}MB RAM
                  </span>
                </div>

                {/* Connection Address Box */}
                <div
                  onClick={(e) => copyToClipboard(connectionAddress, server.id, e)}
                  className="bg-synx-dark/60 hover:bg-synx-dark border border-synx-border rounded-xl p-2.5 mb-4 flex items-center justify-between transition-colors group/addr"
                >
                  <div className="flex flex-col">
                    <span className="text-[10px] text-synx-muted uppercase font-semibold tracking-wider">Java Address</span>
                    <span className="text-xs font-mono font-medium text-white">{connectionAddress}</span>
                  </div>
                  <button className="text-synx-muted group-hover/addr:text-synx-purpleLight p-1">
                    {copiedId === server.id ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
                  </button>
                </div>
              </div>

              {/* Actions Footer */}
              <div className="pt-3 border-t border-synx-border/60 flex items-center justify-between gap-2">
                <div className="flex items-center gap-2">
                  {isStopped && (
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onPowerAction(server.id, 'start');
                      }}
                      className="px-3 py-1.5 rounded-lg bg-emerald-600/20 hover:bg-emerald-600/30 text-emerald-400 border border-emerald-500/30 text-xs font-semibold flex items-center gap-1.5 transition-all"
                    >
                      <Play className="w-3.5 h-3.5 fill-current" />
                      <span>Start</span>
                    </button>
                  )}

                  {isRunning && (
                    <>
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          onPowerAction(server.id, 'stop');
                        }}
                        className="px-3 py-1.5 rounded-lg bg-rose-600/20 hover:bg-rose-600/30 text-rose-400 border border-rose-500/30 text-xs font-semibold flex items-center gap-1.5 transition-all"
                      >
                        <Square className="w-3.5 h-3.5 fill-current" />
                        <span>Stop</span>
                      </button>

                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          onPowerAction(server.id, 'restart');
                        }}
                        className="p-1.5 rounded-lg bg-synx-card hover:bg-synx-cardHover text-synx-muted hover:text-white border border-synx-border transition-all"
                        title="Restart Instance"
                      >
                        <RotateCw className="w-3.5 h-3.5" />
                      </button>
                    </>
                  )}
                </div>

                <span className="text-xs text-synx-purpleLight font-semibold group-hover:translate-x-1 transition-transform flex items-center gap-1">
                  <span>Manage</span>
                  <ExternalLink className="w-3.5 h-3.5" />
                </span>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};

