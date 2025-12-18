#!/usr/bin/env node

/**
 * Disk Allocation Simulator - Backend Server Entry Point
 * This script starts the disk management simulation server
 */

const http = require('http');
const fs = require('fs');
const path = require('path');

// Import the main server logic
const serverModule = require('./server.js');

// Default port
const PORT = process.env.PORT || 8080;

console.log(`
┌────────────────────────────────────────────────────────────┐
│              V-DISK MANAGEMENT SIMULATOR                   │
│              Disk Allocation Simulator                     │
└────────────────────────────────────────────────────────────┘

Starting server on port ${PORT}...
`);

// Create and start the server
const server = http.createServer(serverModule.handleRequest);

server.listen(PORT, () => {
  console.log(`✅ Server listening on http://localhost:${PORT}`);
  console.log(`📁 State file: ${path.join(__dirname, 'disk_state.json')}`);
  console.log(`💡 Press Ctrl+C to stop the server\n`);
  
  // Log initial disk state
  console.log('📊 Initial Disk State:');
  console.log(`   Total Blocks: ${serverModule.diskState.blocks}`);
  console.log(`   Free Blocks: ${serverModule.diskState.state.filter(s => s === 0).length}`);
  console.log(`   Used Blocks: ${serverModule.diskState.state.filter(s => s === 1).length}`);
  console.log(`   Bad Blocks: ${serverModule.diskState.state.filter(s => s === 2).length}`);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\n🔄 Shutting down server...');
  serverModule.saveState();
  console.log('💾 Disk state saved');
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\n\n🔄 Server terminated');
  serverModule.saveState();
  console.log('💾 Disk state saved');
  process.exit(0);
});