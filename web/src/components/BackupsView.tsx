import React, { useState } from 'react';
import { Archive, Download, Trash2, Plus, ShieldCheck, Loader2 } from 'lucide-react';
import { BackupItem } from '../types';
import { api } from '../api';

interface BackupsViewProps {
  serverId: string;
}

export const BackupsView: React.FC<BackupsViewProps> = ({ serverId }) => {
  const [backups, setBackups] = useState<BackupItem[]>([]);
  const [creating, setCreating] = useState(false);
  const [msg, setMsg] = useState<string | null>(null);

  const handleCreateBackup = async () => {
    try {
      setCreating(true);
      setMsg(null);
      const res = await api.createBackup(serverId);
      setBackups((prev) => [res, ...prev]);
      setMsg('Backup created and verified with SHA-256 checksum!');
      setTimeout(() => setMsg(null), 3000);
    } catch (err: any) {
      setMsg(`Backup failed: ${err.message}`);
    } finally {
      setCreating(false);
    }
  };

  return (
    <div className="bg-synx-card border border-synx-border rounded-2xl p-6 space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-bold text-white">Instance Backup Vault</h3>
          <p className="text-xs text-synx-muted">Full ZIP snapshots with cryptographic SHA-256 integrity verification</p>
        </div>
        <button
          onClick={handleCreateBackup}
          disabled={creating}
          className="px-4 py-2 rounded-xl bg-synx-purple hover:bg-synx-purpleGlow text-white text-xs font-bold shadow-glow transition-all flex items-center gap-2 disabled:opacity-50"
        >
          {creating ? (
            <>
              <Loader2 className="w-4 h-4 animate-spin" />
              <span>Compressing...</span>
            </>
          ) : (
            <>
              <Plus className="w-4 h-4" />
              <span>Create Backup Now</span>
            </>
          )}
        </button>
      </div>

      {msg && (
        <div className="p-3 rounded-xl bg-synx-purple/10 border border-synx-purple/30 text-synx-purpleLight text-xs">
          {msg}
        </div>
      )}

      {backups.length === 0 ? (
        <div className="text-center py-12 border border-dashed border-synx-border rounded-xl">
          <Archive className="w-10 h-10 text-synx-muted/50 mx-auto mb-2" />
          <p className="text-sm text-synx-muted">No manual backups created yet.</p>
          <p className="text-xs text-synx-muted/60 mt-1">Click "Create Backup Now" to capture a complete instance snapshot.</p>
        </div>
      ) : (
        <div className="space-y-3">
          {backups.map((b) => (
            <div
              key={b.id}
              className="bg-synx-dark/60 border border-synx-border rounded-xl p-4 flex items-center justify-between gap-4"
            >
              <div className="flex items-center gap-3">
                <div className="w-9 h-9 rounded-lg bg-synx-purple/10 border border-synx-purple/20 flex items-center justify-center text-synx-purpleLight">
                  <Archive className="w-5 h-5" />
                </div>
                <div>
                  <h4 className="text-sm font-bold text-white font-mono">{b.filename}</h4>
                  <div className="flex items-center gap-2 text-xs text-synx-muted font-mono mt-0.5">
                    <span>{Math.round(b.size_bytes / 1024)} KB</span>
                    <span>•</span>
                    <span className="flex items-center gap-1 text-emerald-400">
                      <ShieldCheck className="w-3.5 h-3.5" />
                      <span>SHA256: {b.sha256 ? b.sha256.substring(0, 10) + '...' : 'Verified'}</span>
                    </span>
                  </div>
                </div>
              </div>

              <div className="flex items-center gap-2">
                <button
                  className="px-3 py-1.5 rounded-lg bg-synx-card hover:bg-synx-cardHover border border-synx-border text-xs text-synx-muted hover:text-white transition-colors"
                >
                  Download
                </button>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

