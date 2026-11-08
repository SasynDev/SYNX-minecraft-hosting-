import React, { useState } from 'react';
import { ArrowLeft, Sparkles, Check, Server, Shield, Smartphone, AlertCircle, Loader2 } from 'lucide-react';
import { ServerType } from '../types';

interface ServerWizardProps {
  onBack: () => void;
  onCreated: (serverId: string) => void;
  createInstance: (data: any) => Promise<any>;
}

export const ServerWizard: React.FC<ServerWizardProps> = ({
  onBack,
  onCreated,
  createInstance,
}) => {
  const [name, setName] = useState('Survival SMP');
  const [serverType, setServerType] = useState<ServerType>('paper');
  const [mcVersion, setMcVersion] = useState('1.21.4');
  const [ramMb, setRamMb] = useState(2048);
  const [crackedEnabled, setCrackedEnabled] = useState(true);
  const [crossplayEnabled, setCrossplayEnabled] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const softwareOptions: { type: ServerType; title: string; desc: string; tag: string }[] = [
    { type: 'paper', title: 'Paper', desc: 'High performance Spigot fork with plugin support', tag: 'Recommended' },
    { type: 'purpur', title: 'Purpur', desc: 'Extremely customizable Paper fork with gameplay enhancements', tag: 'Popular' },
    { type: 'vanilla', title: 'Vanilla', desc: 'Official unmodified Mojang server experience', tag: 'Original' },
    { type: 'fabric', title: 'Fabric', desc: 'Lightweight modern modding toolchain', tag: 'Mods' },
    { type: 'forge', title: 'Forge', desc: 'Classic comprehensive Minecraft modpack framework', tag: 'Heavy Mods' },
    { type: 'bedrock', title: 'Bedrock Dedicated', desc: 'Native C++ server for Windows, iOS, Android, and Console', tag: 'Bedrock' },
  ];

  const ramPresets = [
    { label: '1 GB', mb: 1024, desc: 'Small friend group (1-5 players)' },
    { label: '2 GB', mb: 2048, desc: 'Standard SMP with basic plugins' },
    { label: '4 GB', mb: 4096, desc: 'Geyser Crossplay & heavy plugins' },
    { label: '6 GB', mb: 6144, desc: 'Large community or Fabric modpack' },
  ];

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim()) {
      setError('Please provide a server name.');
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const res = await createInstance({
        name,
        server_type: serverType,
        mc_version: mcVersion,
        ram_limit_mb: ramMb,
        cpu_limit_pct: 100,
        cracked_enabled: crackedEnabled,
        crossplay_enabled: crossplayEnabled,
      });

      onCreated(res.id);
    } catch (err: any) {
      setError(err.message || 'Failed to deploy server.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="max-w-4xl mx-auto space-y-6">
      {/* Header */}
      <div className="flex items-center gap-4">
        <button
          onClick={onBack}
          className="p-2 rounded-xl bg-synx-card hover:bg-synx-cardHover border border-synx-border text-synx-muted hover:text-white transition-all"
        >
          <ArrowLeft className="w-5 h-5" />
        </button>
        <div>
          <h2 className="text-2xl font-black text-white tracking-tight">Deploy New Minecraft Server</h2>
          <p className="text-sm text-synx-muted">Configure server software, memory limits, crossplay, and cracked player access</p>
        </div>
      </div>

      {error && (
        <div className="p-4 rounded-xl bg-rose-500/10 border border-rose-500/30 text-rose-300 text-sm flex items-center gap-3">
          <AlertCircle className="w-5 h-5 text-rose-400 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      <form onSubmit={handleSubmit} className="space-y-6">
        {/* Step 1: Name and Version */}
        <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-4">
          <h3 className="text-base font-bold text-white flex items-center gap-2">
            <span className="w-6 h-6 rounded-full bg-synx-purple/20 text-synx-purpleLight flex items-center justify-center text-xs font-bold">1</span>
            <span>Server Name & Version</span>
          </h3>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label className="block text-xs font-medium text-synx-muted mb-1.5">Server Display Name</label>
              <input
                type="text"
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="e.g. DreamSMP / Friends World"
                className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2.5 text-sm text-white focus:outline-none focus:border-synx-purple transition-colors"
                required
              />
            </div>

            <div>
              <label className="block text-xs font-medium text-synx-muted mb-1.5">Minecraft Version</label>
              <input
                type="text"
                value={mcVersion}
                onChange={(e) => setMcVersion(e.target.value)}
                placeholder="1.21.4"
                className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2.5 text-sm text-white focus:outline-none focus:border-synx-purple transition-colors font-mono"
                required
              />
            </div>
          </div>
        </div>

        {/* Step 2: Server Software Selection */}
        <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-4">
          <h3 className="text-base font-bold text-white flex items-center gap-2">
            <span className="w-6 h-6 rounded-full bg-synx-purple/20 text-synx-purpleLight flex items-center justify-center text-xs font-bold">2</span>
            <span>Choose Server Software</span>
          </h3>

          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
            {softwareOptions.map((opt) => {
              const isSelected = serverType === opt.type;
              return (
                <div
                  key={opt.type}
                  onClick={() => setServerType(opt.type)}
                  className={`p-4 rounded-xl border cursor-pointer transition-all flex flex-col justify-between ${
                    isSelected
                      ? 'bg-synx-purple/15 border-synx-purple shadow-glow-sm'
                      : 'bg-synx-dark/50 border-synx-border hover:border-synx-border/80'
                  }`}
                >
                  <div>
                    <div className="flex items-center justify-between mb-1.5">
                      <span className="font-bold text-sm text-white">{opt.title}</span>
                      <span className={`text-[10px] px-2 py-0.5 rounded-full font-semibold ${
                        isSelected ? 'bg-synx-purple text-white' : 'bg-white/5 text-synx-muted'
                      }`}>
                        {opt.tag}
                      </span>
                    </div>
                    <p className="text-xs text-synx-muted leading-relaxed">{opt.desc}</p>
                  </div>
                  {isSelected && (
                    <div className="flex items-center gap-1 text-xs text-synx-purpleLight font-semibold mt-3">
                      <Check className="w-3.5 h-3.5" />
                      <span>Selected</span>
                    </div>
                  )}
                </div>
              );
            })}
          </div>
        </div>

        {/* Step 3: RAM Allocation */}
        <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-4">
          <h3 className="text-base font-bold text-white flex items-center gap-2">
            <span className="w-6 h-6 rounded-full bg-synx-purple/20 text-synx-purpleLight flex items-center justify-center text-xs font-bold">3</span>
            <span>Memory Allocation (Localhost Server Quota)</span>
          </h3>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
            {ramPresets.map((preset) => {
              const isSelected = ramMb === preset.mb;
              return (
                <div
                  key={preset.mb}
                  onClick={() => setRamMb(preset.mb)}
                  className={`p-4 rounded-xl border cursor-pointer transition-all text-center ${
                    isSelected
                      ? 'bg-synx-purple/15 border-synx-purple shadow-glow-sm'
                      : 'bg-synx-dark/50 border-synx-border hover:border-synx-border/80'
                  }`}
                >
                  <div className="font-mono font-bold text-lg text-white mb-1">{preset.label}</div>
                  <div className="text-[11px] text-synx-muted leading-tight">{preset.desc}</div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Step 4: Special Capabilities (Cracked & Crossplay) */}
        <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-4">
          <h3 className="text-base font-bold text-white flex items-center gap-2">
            <span className="w-6 h-6 rounded-full bg-synx-purple/20 text-synx-purpleLight flex items-center justify-center text-xs font-bold">4</span>
            <span>Player Access & Crossplay</span>
          </h3>

          <div className="space-y-4">
            {/* Cracked Players Toggle */}
            <div
              onClick={() => setCrackedEnabled(!crackedEnabled)}
              className={`p-4 rounded-xl border cursor-pointer transition-all flex items-start justify-between gap-4 ${
                crackedEnabled
                  ? 'bg-emerald-500/10 border-emerald-500/40'
                  : 'bg-synx-dark/50 border-synx-border'
              }`}
            >
              <div className="flex items-start gap-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center shrink-0 ${
                  crackedEnabled ? 'bg-emerald-500/20 text-emerald-400' : 'bg-synx-card text-synx-muted'
                }`}>
                  <Shield className="w-5 h-5" />
                </div>
                <div>
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className="font-bold text-sm text-white">Enable Cracked (Offline-Mode) Players</span>
                    <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 font-bold uppercase tracking-wider">
                      TLauncher Ready
                    </span>
                  </div>
                  <p className="text-xs text-synx-muted">
                    Allows players without an official Microsoft account to join. Sets <code className="text-emerald-400 font-mono">online-mode=false</code> with automatic skin support.
                  </p>
                </div>
              </div>

              <div className={`w-6 h-6 rounded-md border flex items-center justify-center shrink-0 transition-colors ${
                crackedEnabled ? 'bg-emerald-500 border-emerald-400 text-white' : 'border-synx-border bg-synx-card'
              }`}>
                {crackedEnabled && <Check className="w-4 h-4 stroke-[3]" />}
              </div>
            </div>

            {/* Bedrock Crossplay Toggle */}
            <div
              onClick={() => setCrossplayEnabled(!crossplayEnabled)}
              className={`p-4 rounded-xl border cursor-pointer transition-all flex items-start justify-between gap-4 ${
                crossplayEnabled
                  ? 'bg-synx-purple/15 border-synx-purple'
                  : 'bg-synx-dark/50 border-synx-border'
              }`}
            >
              <div className="flex items-start gap-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center shrink-0 ${
                  crossplayEnabled ? 'bg-synx-purple text-white shadow-glow-sm' : 'bg-synx-card text-synx-muted'
                }`}>
                  <Smartphone className="w-5 h-5" />
                </div>
                <div>
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className="font-bold text-sm text-white">Java + Bedrock Crossplay (GeyserMC)</span>
                    <span className="text-[10px] px-2 py-0.5 rounded-full bg-synx-purple/20 text-synx-purpleLight font-bold uppercase tracking-wider">
                      Mobile & Console
                    </span>
                  </div>
                  <p className="text-xs text-synx-muted">
                    Enables mobile (iOS/Android), Xbox, PlayStation, and Switch players to join this Java server on Bedrock UDP port 19132.
                  </p>
                </div>
              </div>

              <div className={`w-6 h-6 rounded-md border flex items-center justify-center shrink-0 transition-colors ${
                crossplayEnabled ? 'bg-synx-purple border-synx-purple text-white' : 'border-synx-border bg-synx-card'
              }`}>
                {crossplayEnabled && <Check className="w-4 h-4 stroke-[3]" />}
              </div>
            </div>
          </div>
        </div>

        {/* Deploy Button */}
        <div className="flex justify-end gap-3 pt-2">
          <button
            type="button"
            onClick={onBack}
            className="px-5 py-2.5 rounded-xl border border-synx-border text-synx-muted hover:text-white text-sm font-semibold transition-all"
          >
            Cancel
          </button>

          <button
            type="submit"
            disabled={loading}
            className="px-7 py-2.5 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white text-sm font-bold shadow-glow transition-all flex items-center gap-2 disabled:opacity-50"
          >
            {loading ? (
              <>
                <Loader2 className="w-4 h-4 animate-spin" />
                <span>Deploying Instance...</span>
              </>
            ) : (
              <>
                <Sparkles className="w-4 h-4" />
                <span>Deploy Minecraft Server</span>
              </>
            )}
          </button>
        </div>
      </form>
    </div>
  );
};

