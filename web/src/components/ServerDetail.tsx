import React, { useState, useEffect } from 'react';
import {
  ArrowLeft,
  Play,
  Square,
  RotateCw,
  Skull,
  Copy,
  Check,
  Terminal,
  Settings,
  Archive,
  Cpu,
  HardDrive,
  Clock,
  Shield,
  Smartphone,
  Loader2,
  Globe,
  Wifi,
  Share2,
  Edit2,
  Info,
  ChevronDown,
  ChevronUp,
} from 'lucide-react';
import { InstanceDetail } from '../types';
import { api } from '../api';
import { ConsoleView } from './ConsoleView';
import { PropertiesEditor } from './PropertiesEditor';
import { BackupsView } from './BackupsView';

interface ServerDetailProps {
  serverId: string;
  onBack: () => void;
}

export const ServerDetail: React.FC<ServerDetailProps> = ({ serverId, onBack }) => {
  const [server, setServer] = useState<InstanceDetail | null>(null);
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'console' | 'properties' | 'backups'>('console');
  const [copiedJava, setCopiedJava] = useState(false);
  const [copiedBedrock, setCopiedBedrock] = useState(false);
  const [copiedPublic, setCopiedPublic] = useState(false);
  const [copiedLan, setCopiedLan] = useState(false);
  const [actionLoading, setActionLoading] = useState(false);
  const [customDomain, setCustomDomain] = useState<string>(() => {
    return localStorage.getItem(`synx_custom_domain_${serverId}`) || '';
  });
  const [editingDomain, setEditingDomain] = useState(false);
  const [tempDomain, setTempDomain] = useState(customDomain);
  const [showNetworkGuide, setShowNetworkGuide] = useState(false);

  useEffect(() => {
    loadServer();
    const interval = setInterval(loadServer, 2000);
    return () => clearInterval(interval);
  }, [serverId]);

  const loadServer = async () => {
    try {
      const data = await api.getInstance(serverId);
      setServer(data);
    } catch (err) {
      // silent
    } finally {
      setLoading(false);
    }
  };

  const handlePowerAction = async (action: 'start' | 'stop' | 'restart' | 'kill') => {
    try {
      setActionLoading(true);
      await api.sendPowerAction(serverId, action);
      await loadServer();
    } catch (err) {
      alert(`Power action failed: ${err}`);
    } finally {
      setActionLoading(false);
    }
  };

  if (loading || !server) {
    return (
      <div className="p-20 text-center flex items-center justify-center gap-2 text-synx-muted">
        <Loader2 className="w-5 h-5 animate-spin text-synx-purpleLight" />
        <span>Loading server instance...</span>
      </div>
    );
  }

  const isRunning = server.status === 'running';
  const isStopped = server.status === 'stopped';
  const isStarting = server.status === 'starting';

  // Dynamic host resolution without exposing sensitive home network IP
  const currentHost = window.location.hostname || 'localhost';
  const isLocal = currentHost === 'localhost' || currentHost === '127.0.0.1';
  const defaultHost = isLocal ? 'localhost' : currentHost;

  const lanAddr = `${defaultHost}:${server.port}`;
  const localAddr = `localhost:${server.port}`;
  const publicAddr = customDomain || `${defaultHost}:${server.port}`;
  const bedrockAddr = `${customDomain ? customDomain.split(':')[0] : defaultHost}:19132`;

  const copyText = (text: string, type: 'java' | 'bedrock' | 'public' | 'lan') => {
    navigator.clipboard.writeText(text);
    if (type === 'java') {
      setCopiedJava(true);
      setTimeout(() => setCopiedJava(false), 2000);
    } else if (type === 'bedrock') {
      setCopiedBedrock(true);
      setTimeout(() => setCopiedBedrock(false), 2000);
    } else if (type === 'public') {
      setCopiedPublic(true);
      setTimeout(() => setCopiedPublic(false), 2000);
    } else if (type === 'lan') {
      setCopiedLan(true);
      setTimeout(() => setCopiedLan(false), 2000);
    }
  };

  const saveCustomDomain = (e: React.FormEvent) => {
    e.preventDefault();
    const clean = tempDomain.trim();
    setCustomDomain(clean);
    localStorage.setItem(`synx_custom_domain_${serverId}`, clean);
    setEditingDomain(false);
  };

  return (
    <div className="space-y-6">
      {/* Top Bar: Navigation, Status, and Controls */}
      <div className="bg-synx-card border border-synx-border rounded-2xl p-6 shadow-xl space-y-5">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center gap-4">
            <button
              onClick={onBack}
              className="p-2.5 rounded-xl bg-synx-dark hover:bg-synx-dark/80 border border-synx-border text-synx-muted hover:text-white transition-all"
            >
              <ArrowLeft className="w-5 h-5" />
            </button>
            <div>
              <div className="flex items-center gap-3">
                <h2 className="text-2xl font-black text-white tracking-tight">{server.name}</h2>
                <span
                  className={`text-xs px-3 py-1 rounded-full font-bold uppercase tracking-wider flex items-center gap-1.5 ${
                    isRunning
                      ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                      : isStarting
                      ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30 animate-pulse'
                      : 'bg-rose-500/10 text-rose-400 border border-rose-500/30'
                  }`}
                >
                  <span
                    className={`w-2 h-2 rounded-full ${
                      isRunning ? 'bg-emerald-400' : isStarting ? 'bg-amber-400' : 'bg-rose-400'
                    }`}
                  />
                  {server.status}
                </span>
              </div>
              <p className="text-xs text-synx-muted font-mono mt-0.5">
                UUID: {server.id} | Slug: {server.identifier}
              </p>
            </div>
          </div>

          {/* Action Buttons */}
          <div className="flex items-center gap-2">
            {isStopped && (
              <button
                onClick={() => handlePowerAction('start')}
                disabled={actionLoading}
                className="px-5 py-2.5 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-white font-bold text-xs shadow-lg shadow-emerald-600/20 flex items-center gap-2 transition-all disabled:opacity-50"
              >
                <Play className="w-4 h-4 fill-current" />
                <span>Start Server</span>
              </button>
            )}

            {isRunning && (
              <>
                <button
                  onClick={() => handlePowerAction('stop')}
                  disabled={actionLoading}
                  className="px-4 py-2.5 rounded-xl bg-rose-600 hover:bg-rose-500 text-white font-bold text-xs shadow-lg shadow-rose-600/20 flex items-center gap-2 transition-all disabled:opacity-50"
                >
                  <Square className="w-4 h-4 fill-current" />
                  <span>Stop</span>
                </button>

                <button
                  onClick={() => handlePowerAction('restart')}
                  disabled={actionLoading}
                  className="px-4 py-2.5 rounded-xl bg-synx-dark hover:bg-synx-dark/80 text-white font-bold text-xs border border-synx-border flex items-center gap-2 transition-all disabled:opacity-50"
                >
                  <RotateCw className="w-4 h-4" />
                  <span>Restart</span>
                </button>

                <button
                  onClick={() => handlePowerAction('kill')}
                  disabled={actionLoading}
                  className="p-2.5 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-400 border border-rose-500/30 transition-all"
                  title="Force Kill (Emergency)"
                >
                  <Skull className="w-4 h-4" />
                </button>
              </>
            )}
          </div>
        </div>

        {/* Connection Addresses & Specs Grid */}
        <div className="space-y-3 pt-4 border-t border-synx-border/60">
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
            {/* 1. Public / Custom Domain IP (For Friends Anywhere over Internet) */}
            <div className="bg-synx-dark/80 border border-synx-purple/30 rounded-xl p-3 flex flex-col justify-between transition-colors relative overflow-hidden group">
              <div className="flex items-center justify-between">
                <div className="text-[10px] text-synx-purpleLight font-bold uppercase tracking-wider flex items-center gap-1.5">
                  <Globe className="w-3.5 h-3.5 text-synx-purpleLight" />
                  <span>Public / Custom Domain (For Friends)</span>
                </div>
                <button
                  onClick={() => {
                    setTempDomain(customDomain);
                    setEditingDomain(!editingDomain);
                  }}
                  className="text-xs text-synx-muted hover:text-white p-1 rounded hover:bg-synx-card"
                  title="Configure custom domain or IP"
                >
                  <Edit2 className="w-3 h-3" />
                </button>
              </div>

              {editingDomain ? (
                <form onSubmit={saveCustomDomain} className="mt-2 space-y-2">
                  <input
                    type="text"
                    value={tempDomain}
                    onChange={(e) => setTempDomain(e.target.value)}
                    placeholder="e.g. play.yourdomain.com"
                    className="w-full bg-synx-card text-white text-xs px-2.5 py-1.5 rounded-lg border border-synx-purple/50 focus:outline-none"
                    autoFocus
                  />
                  <div className="flex gap-2">
                    <button
                      type="submit"
                      className="px-2.5 py-1 bg-synx-purple text-white text-[11px] font-bold rounded"
                    >
                      Save
                    </button>
                    <button
                      type="button"
                      onClick={() => setEditingDomain(false)}
                      className="px-2 py-1 text-synx-muted hover:text-white text-[11px]"
                    >
                      Cancel
                    </button>
                  </div>
                </form>
              ) : (
                <div
                  onClick={() => copyText(publicAddr, 'public')}
                  className="cursor-pointer flex items-center justify-between mt-1"
                >
                  <div className="text-xs font-mono font-bold text-white truncate max-w-[180px]">
                    {publicAddr}
                  </div>
                  <button className="text-synx-muted group-hover:text-synx-purpleLight p-1">
                    {copiedPublic ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
                  </button>
                </div>
              )}
            </div>

            {/* 2. Local Wi-Fi Address (For Friends on Same Wi-Fi / LAN) */}
            <div
              onClick={() => copyText(lanAddr, 'lan')}
              className="bg-synx-dark/60 hover:bg-synx-dark border border-synx-border rounded-xl p-3 flex items-center justify-between cursor-pointer transition-colors"
            >
              <div>
                <div className="text-[10px] text-synx-muted font-bold uppercase tracking-wider flex items-center gap-1">
                  <Wifi className="w-3 h-3 text-cyan-400" />
                  <span>Local Wi-Fi IP (Same Router)</span>
                </div>
                <div className="text-xs font-mono font-bold text-white mt-0.5">{lanAddr}</div>
              </div>
              <button className="text-synx-muted p-1">
                {copiedLan ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
              </button>
            </div>

            {/* 3. Direct Connection (Your PC) */}
            <div
              onClick={() => copyText(localAddr, 'java')}
              className="bg-synx-dark/60 hover:bg-synx-dark border border-synx-border rounded-xl p-3 flex items-center justify-between cursor-pointer transition-colors"
            >
              <div>
                <div className="text-[10px] text-synx-muted font-bold uppercase tracking-wider">Direct Connect (Host PC)</div>
                <div className="text-xs font-mono font-bold text-white mt-0.5">{localAddr}</div>
              </div>
              <button className="text-synx-muted p-1">
                {copiedJava ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
              </button>
            </div>

            {/* 4. Bedrock / Mobile Port */}
            <div
              onClick={() => copyText(bedrockAddr, 'bedrock')}
              className="bg-synx-dark/60 hover:bg-synx-dark border border-synx-border rounded-xl p-3 flex items-center justify-between cursor-pointer transition-colors"
            >
              <div>
                <div className="text-[10px] text-synx-muted font-bold uppercase tracking-wider flex items-center gap-1">
                  <Smartphone className="w-3 h-3 text-synx-purpleLight" />
                  <span>Bedrock / Mobile Port</span>
                </div>
                <div className="text-xs font-mono font-bold text-white mt-0.5">{bedrockAddr}</div>
              </div>
              <button className="text-synx-muted p-1">
                {copiedBedrock ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
              </button>
            </div>
          </div>

          {/* Expandable Help: How Friends Join */}
          <div className="border border-synx-border rounded-xl bg-synx-dark/40 overflow-hidden">
            <button
              onClick={() => setShowNetworkGuide(!showNetworkGuide)}
              className="w-full px-4 py-2.5 flex items-center justify-between text-xs text-synx-muted hover:text-white transition-colors"
            >
              <span className="flex items-center gap-2 font-semibold">
                <Info className="w-4 h-4 text-synx-purpleLight" />
                <span>Friends join kaise karenge? (Connection Guide)</span>
              </span>
              {showNetworkGuide ? <ChevronUp className="w-4 h-4" /> : <ChevronDown className="w-4 h-4" />}
            </button>

            {showNetworkGuide && (
              <div className="p-4 border-t border-synx-border space-y-3 text-xs text-synx-muted">
                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                  <div className="p-3 rounded-lg bg-synx-card border border-synx-border space-y-1.5">
                    <div className="font-bold text-white flex items-center gap-1.5">
                      <Wifi className="w-3.5 h-3.5 text-cyan-400" />
                      <span>1. Friends on Same Wi-Fi / Ghar Par:</span>
                    </div>
                    <p className="text-[11px] leading-relaxed">
                      Jo friends aapke ghar ke Wi-Fi ya local network se jude hain, wo seedha ye IP dalein:
                    </p>
                    <code className="block p-1.5 rounded bg-black/40 text-emerald-400 font-mono text-xs select-all">
                      {lanAddr}
                    </code>
                  </div>

                  <div className="p-3 rounded-lg bg-synx-card border border-synx-border space-y-1.5">
                    <div className="font-bold text-white flex items-center gap-1.5">
                      <Globe className="w-3.5 h-3.5 text-synx-purpleLight" />
                      <span>2. Friends over Internet (Bahar se):</span>
                    </div>
                    <p className="text-[11px] leading-relaxed">
                      Wi-Fi router me Port <strong>{server.port}</strong> ko forward karein to <strong>{defaultHost}</strong> (ya PC local IPv4). Phir friends ko share karein:
                    </p>
                    <code className="block p-1.5 rounded bg-black/40 text-synx-purpleLight font-mono text-xs select-all">
                      {publicAddr}
                    </code>
                  </div>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Tabs Selector */}
      <div className="flex items-center gap-2 border-b border-synx-border pb-3">
        <button
          onClick={() => setActiveTab('console')}
          className={`px-4 py-2 rounded-xl text-xs font-bold flex items-center gap-2 transition-all ${
            activeTab === 'console'
              ? 'bg-synx-purple text-white shadow-glow'
              : 'text-synx-muted hover:text-white bg-synx-card/50'
          }`}
        >
          <Terminal className="w-4 h-4" />
          <span>Live Console</span>
        </button>

        <button
          onClick={() => setActiveTab('properties')}
          className={`px-4 py-2 rounded-xl text-xs font-bold flex items-center gap-2 transition-all ${
            activeTab === 'properties'
              ? 'bg-synx-purple text-white shadow-glow'
              : 'text-synx-muted hover:text-white bg-synx-card/50'
          }`}
        >
          <Settings className="w-4 h-4" />
          <span>Server Properties & Cracked Mode</span>
        </button>

        <button
          onClick={() => setActiveTab('backups')}
          className={`px-4 py-2 rounded-xl text-xs font-bold flex items-center gap-2 transition-all ${
            activeTab === 'backups'
              ? 'bg-synx-purple text-white shadow-glow'
              : 'text-synx-muted hover:text-white bg-synx-card/50'
          }`}
        >
          <Archive className="w-4 h-4" />
          <span>Backups Vault</span>
        </button>
      </div>

      {/* Tab Panels */}
      <div>
        {activeTab === 'console' && <ConsoleView serverId={server.id} />}
        {activeTab === 'properties' && <PropertiesEditor serverId={server.id} />}
        {activeTab === 'backups' && <BackupsView serverId={server.id} />}
      </div>
    </div>
  );
};

