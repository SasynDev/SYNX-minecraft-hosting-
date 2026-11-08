import React, { useState, useEffect } from 'react';
import { Save, Shield, ShieldCheck, Check, AlertCircle, Loader2 } from 'lucide-react';
import { api } from '../api';

interface PropertiesEditorProps {
  serverId: string;
}

export const PropertiesEditor: React.FC<PropertiesEditorProps> = ({ serverId }) => {
  const [props, setProps] = useState<Record<string, string>>({});
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [savedSuccess, setSavedSuccess] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadProperties();
  }, [serverId]);

  const loadProperties = async () => {
    try {
      setLoading(true);
      const data = await api.getProperties(serverId);
      setProps(data);
    } catch (err: any) {
      setError(err.message || 'Failed to load properties.');
    } finally {
      setLoading(false);
    }
  };

  const handleToggleCracked = async () => {
    const isCurrentlyCracked = props['online-mode'] === 'false';
    const newCrackedState = !isCurrentlyCracked;

    try {
      setSaving(true);
      await api.setCrackedMode(serverId, newCrackedState);
      setProps((prev) => ({ ...prev, 'online-mode': newCrackedState ? 'false' : 'true' }));
      setSavedSuccess(true);
      setTimeout(() => setSavedSuccess(false), 2500);
    } catch (err: any) {
      setError(err.message || 'Failed to toggle cracked mode.');
    } finally {
      setSaving(false);
    }
  };

  const handleSave = async (e: React.FormEvent) => {
    e.preventDefault();
    try {
      setSaving(true);
      setError(null);
      await api.updateProperties(serverId, props);
      setSavedSuccess(true);
      setTimeout(() => setSavedSuccess(false), 2500);
    } catch (err: any) {
      setError(err.message || 'Failed to save properties.');
    } finally {
      setSaving(false);
    }
  };

  if (loading) {
    return (
      <div className="p-12 text-center text-synx-muted flex items-center justify-center gap-2">
        <Loader2 className="w-5 h-5 animate-spin text-synx-purpleLight" />
        <span>Loading server properties...</span>
      </div>
    );
  }

  const isCracked = props['online-mode'] === 'false';

  return (
    <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-bold text-white">Server Properties & Player Access</h3>
          <p className="text-xs text-synx-muted">Changes require a server restart to take effect</p>
        </div>
        {savedSuccess && (
          <div className="flex items-center gap-1.5 text-xs text-emerald-400 bg-emerald-500/10 px-3 py-1.5 rounded-lg border border-emerald-500/20">
            <Check className="w-4 h-4" />
            <span>Saved successfully!</span>
          </div>
        )}
      </div>

      {error && (
        <div className="p-4 rounded-xl bg-rose-500/10 border border-rose-500/30 text-rose-300 text-xs flex items-center gap-2">
          <AlertCircle className="w-4 h-4 text-rose-400 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Featured: Cracked (Offline-Mode) Toggle Card */}
      <div
        onClick={handleToggleCracked}
        className={`p-5 rounded-2xl border cursor-pointer transition-all flex items-start justify-between gap-4 ${
          isCracked
            ? 'bg-emerald-500/10 border-emerald-500/40'
            : 'bg-synx-dark/50 border-synx-border hover:border-synx-purple/50'
        }`}
      >
        <div className="flex items-start gap-3.5">
          <div className={`w-10 h-10 rounded-xl flex items-center justify-center shrink-0 ${
            isCracked ? 'bg-emerald-500/20 text-emerald-400' : 'bg-synx-card text-synx-muted'
          }`}>
            <Shield className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2 mb-1">
              <span className="font-bold text-sm text-white">Cracked (Offline-Mode) Access</span>
              <span className={`text-[10px] px-2 py-0.5 rounded-full font-bold uppercase tracking-wider ${
                isCracked ? 'bg-emerald-500/20 text-emerald-300' : 'bg-rose-500/20 text-rose-300'
              }`}>
                {isCracked ? 'Cracked Allowed (TLauncher)' : 'Official Microsoft Accounts Only'}
              </span>
            </div>
            <p className="text-xs text-synx-muted leading-relaxed">
              When enabled (<code className="text-emerald-400 font-mono">online-mode=false</code>), players using TLauncher or offline clients can join without Microsoft session check. SkinRestorer is automatically configured.
            </p>
          </div>
        </div>

        <div className={`w-6 h-6 rounded-md border flex items-center justify-center shrink-0 transition-colors ${
          isCracked ? 'bg-emerald-500 border-emerald-400 text-white' : 'border-synx-border bg-synx-card'
        }`}>
          {isCracked && <Check className="w-4 h-4 stroke-[3]" />}
        </div>
      </div>

      <form onSubmit={handleSave} className="space-y-4">
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {/* MOTD */}
          <div className="md:col-span-2">
            <label className="block text-xs font-medium text-synx-muted mb-1.5">Server Message of the Day (MOTD)</label>
            <input
              type="text"
              value={props['motd'] || ''}
              onChange={(e) => setProps({ ...props, motd: e.target.value })}
              className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-synx-purple"
            />
          </div>

          {/* Max Players */}
          <div>
            <label className="block text-xs font-medium text-synx-muted mb-1.5">Max Players</label>
            <input
              type="number"
              value={props['max-players'] || '20'}
              onChange={(e) => setProps({ ...props, 'max-players': e.target.value })}
              className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-synx-purple font-mono"
            />
          </div>

          {/* Difficulty */}
          <div>
            <label className="block text-xs font-medium text-synx-muted mb-1.5">Game Difficulty</label>
            <select
              value={props['difficulty'] || 'easy'}
              onChange={(e) => setProps({ ...props, difficulty: e.target.value })}
              className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-synx-purple capitalize"
            >
              <option value="peaceful">Peaceful</option>
              <option value="easy">Easy</option>
              <option value="normal">Normal</option>
              <option value="hard">Hard</option>
            </select>
          </div>

          {/* Gamemode */}
          <div>
            <label className="block text-xs font-medium text-synx-muted mb-1.5">Default Gamemode</label>
            <select
              value={props['gamemode'] || 'survival'}
              onChange={(e) => setProps({ ...props, gamemode: e.target.value })}
              className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-synx-purple capitalize"
            >
              <option value="survival">Survival</option>
              <option value="creative">Creative</option>
              <option value="adventure">Adventure</option>
            </select>
          </div>

          {/* PvP Toggle */}
          <div>
            <label className="block text-xs font-medium text-synx-muted mb-1.5">Player vs Player (PvP)</label>
            <select
              value={props['pvp'] || 'true'}
              onChange={(e) => setProps({ ...props, pvp: e.target.value })}
              className="w-full bg-synx-dark border border-synx-border rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-synx-purple"
            >
              <option value="true">Enabled (Combat Allowed)</option>
              <option value="false">Disabled (Safe Mode)</option>
            </select>
          </div>
        </div>

        <div className="flex justify-end pt-3">
          <button
            type="submit"
            disabled={saving}
            className="px-6 py-2.5 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white text-xs font-bold shadow-glow transition-all flex items-center gap-2 disabled:opacity-50"
          >
            {saving ? (
              <>
                <Loader2 className="w-4 h-4 animate-spin" />
                <span>Saving Properties...</span>
              </>
            ) : (
              <>
                <Save className="w-4 h-4" />
                <span>Save Configuration</span>
              </>
            )}
          </button>
        </div>
      </form>
    </div>
  );
};

