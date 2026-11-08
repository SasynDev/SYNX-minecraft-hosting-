import React from 'react';
import { Server, Activity, ShieldCheck, Plus, Cpu, HardDrive } from 'lucide-react';
import { HostMetrics } from '../types';

interface NavbarProps {
  currentTab: 'servers' | 'create' | 'node';
  onSelectTab: (tab: 'servers' | 'create' | 'node') => void;
  metrics: HostMetrics | null;
}

export const Navbar: React.FC<NavbarProps> = ({ currentTab, onSelectTab, metrics }) => {
  return (
    <header className="sticky top-0 z-50 bg-synx-dark/80 backdrop-blur-md border-b border-synx-border">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        
        {/* Brand */}
        <div className="flex items-center gap-3 cursor-pointer" onClick={() => onSelectTab('servers')}>
          <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-synx-purple to-synx-purpleGlow flex items-center justify-center shadow-glow">
            <span className="font-black text-xl text-white tracking-wider">S</span>
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-extrabold text-lg text-white tracking-tight">SYNX</span>
              <span className="text-xs px-2 py-0.5 rounded-full bg-synx-purple/20 text-synx-purpleLight border border-synx-purple/30 font-semibold tracking-wider uppercase">HOSTING</span>
            </div>
            <p className="text-[11px] text-synx-muted tracking-wide -mt-0.5">Independent Orchestration Engine</p>
          </div>
        </div>

        {/* Navigation */}
        <nav className="flex items-center gap-1.5 sm:gap-2">
          <button
            onClick={() => onSelectTab('servers')}
            className={`px-3.5 py-1.5 rounded-lg text-sm font-medium flex items-center gap-2 transition-all ${
              currentTab === 'servers'
                ? 'bg-synx-card text-white border border-synx-purple shadow-glow-sm'
                : 'text-synx-muted hover:text-white hover:bg-synx-card/50'
            }`}
          >
            <Server className="w-4 h-4 text-synx-purpleLight" />
            <span>Servers</span>
          </button>

          <button
            onClick={() => onSelectTab('create')}
            className={`px-3.5 py-1.5 rounded-lg text-sm font-medium flex items-center gap-2 transition-all ${
              currentTab === 'create'
                ? 'bg-synx-purple text-white shadow-glow'
                : 'bg-synx-purple/10 text-synx-purpleLight hover:bg-synx-purple/20 border border-synx-purple/30'
            }`}
          >
            <Plus className="w-4 h-4" />
            <span>Create Server</span>
          </button>
        </nav>

        {/* Host Node Hardware Status Pill */}
        {metrics && (
          <div className="hidden md:flex items-center gap-3 text-xs bg-synx-card px-3.5 py-1.5 rounded-full border border-synx-border">
            <div className="flex items-center gap-1.5 text-synx-muted">
              <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
              <span className="text-white font-medium">Localhost Minecraft Node</span>
            </div>
            <span className="text-synx-border">|</span>
            <div className="flex items-center gap-1 text-synx-muted">
              <Cpu className="w-3.5 h-3.5 text-synx-purpleLight" />
              <span>CPU: <strong className="text-white font-mono">{metrics.cpu_usage_pct.toFixed(1)}%</strong></span>
            </div>
            <span className="text-synx-border">|</span>
            <div className="flex items-center gap-1 text-synx-muted">
              <HardDrive className="w-3.5 h-3.5 text-cyan-400" />
              <span>RAM: <strong className="text-white font-mono">{Math.round(metrics.ram_used_mb / 1024 * 10) / 10}GB</strong></span>
            </div>
          </div>
        )}

      </div>
    </header>
  );
};

