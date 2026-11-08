import React, { useState, useEffect } from 'react';
import { Navbar } from './components/Navbar';
import { ServerList } from './components/ServerList';
import { ServerWizard } from './components/ServerWizard';
import { ServerDetail } from './components/ServerDetail';
import { InstanceSummary, HostMetrics } from './types';
import { api } from './api';

export const App: React.FC = () => {
  const [currentTab, setCurrentTab] = useState<'servers' | 'create' | 'node'>('servers');
  const [selectedServerId, setSelectedServerId] = useState<string | null>(null);
  const [instances, setInstances] = useState<InstanceSummary[]>([]);
  const [metrics, setMetrics] = useState<HostMetrics | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 3000);
    return () => clearInterval(interval);
  }, []);

  const loadData = async () => {
    try {
      const [insts, mets] = await Promise.all([
        api.getInstances(),
        api.getHostMetrics().catch(() => null),
      ]);
      setInstances(insts || []);
      setMetrics(mets);
    } catch (err) {
      // silent
    } finally {
      setLoading(false);
    }
  };

  const handlePowerAction = async (id: string, action: 'start' | 'stop' | 'restart') => {
    try {
      await api.sendPowerAction(id, action);
      await loadData();
    } catch (err) {
      alert(`Action failed: ${err}`);
    }
  };

  const handleServerCreated = (newId: string) => {
    setSelectedServerId(newId);
    setCurrentTab('servers');
    loadData();
  };

  return (
    <div className="min-h-screen bg-synx-dark text-synx-text flex flex-col font-['Plus_Jakarta_Sans',sans-serif]">
      {/* Top Navbar */}
      <Navbar
        currentTab={currentTab}
        onSelectTab={(tab) => {
          setSelectedServerId(null);
          setCurrentTab(tab);
        }}
        metrics={metrics}
      />

      {/* Main Body */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {selectedServerId ? (
          <ServerDetail
            serverId={selectedServerId}
            onBack={() => {
              setSelectedServerId(null);
              loadData();
            }}
          />
        ) : currentTab === 'create' ? (
          <ServerWizard
            onBack={() => setCurrentTab('servers')}
            onCreated={handleServerCreated}
            createInstance={api.createInstance}
          />
        ) : (
          <ServerList
            instances={instances}
            onSelectServer={(id) => setSelectedServerId(id)}
            onPowerAction={handlePowerAction}
            onCreateNew={() => setCurrentTab('create')}
          />
        )}
      </main>

      {/* Footer */}
      <footer className="border-t border-synx-border bg-synx-dark/80 py-6 mt-auto">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col sm:flex-row items-center justify-between gap-4 text-xs text-synx-muted">
          <div className="flex items-center gap-2">
            <span className="font-bold text-white tracking-wider">SYNX HOSTING</span>
            <span>•</span>
            <span>Localhost Minecraft Server (Windows 11)</span>
          </div>
          <div>
            <span>PostgreSQL & C++20 Core Orchestration Architecture</span>
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;

