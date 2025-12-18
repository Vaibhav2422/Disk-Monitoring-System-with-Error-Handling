const http = require('http');
const fs = require('fs');
const path = require('path');
const os = require('os');

// Set port to 3001 to avoid conflicts
const PORT = 3003;

// Simple disk simulation
let diskState = {
  blocks: 1024,
  state: new Array(1024).fill(0), // 0=free, 1=used, 2=bad
  owner: new Array(1024).fill(-1),
  files: {},
  nextFileId: 1,
  logs: []
};

// Load existing state
const stateFile = path.join(__dirname, 'disk_state.json');
if (fs.existsSync(stateFile)) {
  try {
    const data = JSON.parse(fs.readFileSync(stateFile, 'utf8'));
    diskState = { ...diskState, ...data };
  } catch (e) {
    console.log('Could not load state, starting fresh');
  }
}

function saveState() {
  fs.writeFileSync(stateFile, JSON.stringify(diskState, null, 2));
}

function log(message) {
  const timestamp = new Date().toISOString();
  diskState.logs.push(`${timestamp}: ${message}`);
  if (diskState.logs.length > 100) {
    diskState.logs.shift();
  }
  console.log(message);
}

function allocateContiguous(size) {
  let streak = 0;
  let start = -1;
  
  for (let i = 0; i < diskState.blocks; i++) {
    if (diskState.state[i] === 0) { // free
      if (streak === 0) start = i;
      streak++;
      if (streak >= size) {
        const fileId = diskState.nextFileId++;
        diskState.files[fileId] = { id: fileId, status: 'active' };
        
        for (let j = start; j < start + size; j++) {
          diskState.state[j] = 1; // used
          diskState.owner[j] = fileId;
        }
        
        log(`allocate_contiguous: id=${fileId} size=${size} start=${start}`);
        saveState();
        return { fileId };
      }
    } else {
      streak = 0;
    }
  }
  throw new Error('No contiguous space available');
}

function allocateFragmented(size) {
  const freeBlocks = diskState.state.filter(s => s === 0).length;
  if (freeBlocks < size) {
    throw new Error('Not enough free blocks');
  }
  
  const fileId = diskState.nextFileId++;
  diskState.files[fileId] = { id: fileId, status: 'active' };
  
  let allocated = 0;
  for (let i = 0; i < diskState.blocks && allocated < size; i++) {
    if (diskState.state[i] === 0) {
      diskState.state[i] = 1;
      diskState.owner[i] = fileId;
      allocated++;
    }
  }
  
  log(`allocate_fragmented: id=${fileId} size=${size}`);
  saveState();
  return { fileId };
}

function allocateCustom(size, strategy) {
  const holes = [];
  let i = 0;
  
  while (i < diskState.blocks) {
    while (i < diskState.blocks && diskState.state[i] !== 0) i++;
    if (i >= diskState.blocks) break;
    
    const start = i;
    let length = 0;
    while (i < diskState.blocks && diskState.state[i] === 0) {
      i++;
      length++;
    }
    
    if (length >= size) {
      holes.push({ start, length });
    }
  }
  
  if (holes.length === 0) {
    throw new Error('No suitable holes found');
  }
  
  let choice = -1;
  if (strategy === 'best-fit') {
    let bestLength = Infinity;
    for (let i = 0; i < holes.length; i++) {
      if (holes[i].length >= size && holes[i].length < bestLength) {
        bestLength = holes[i].length;
        choice = i;
      }
    }
  } else if (strategy === 'worst-fit') {
    let worstLength = -1;
    for (let i = 0; i < holes.length; i++) {
      if (holes[i].length >= size && holes[i].length > worstLength) {
        worstLength = holes[i].length;
        choice = i;
      }
    }
  } else { // first-fit
    for (let i = 0; i < holes.length; i++) {
      if (holes[i].length >= size) {
        choice = i;
        break;
      }
    }
  }
  
  if (choice === -1) {
    throw new Error('No suitable hole found');
  }
  
  const start = holes[choice].start;
  const fileId = diskState.nextFileId++;
  diskState.files[fileId] = { id: fileId, status: 'active' };
  
  for (let j = start; j < start + size; j++) {
    diskState.state[j] = 1;
    diskState.owner[j] = fileId;
  }
  
  log(`allocate_custom: id=${fileId} size=${size} strategy=${strategy} start=${start}`);
  saveState();
  return { fileId, strategy };
}

function defragment() {
  let writeIdx = 0;
  for (let readIdx = 0; readIdx < diskState.blocks; readIdx++) {
    if (diskState.state[readIdx] === 1) { // used
      if (writeIdx !== readIdx) {
        diskState.state[writeIdx] = 1;
        diskState.owner[writeIdx] = diskState.owner[readIdx];
        diskState.state[readIdx] = 0;
        diskState.owner[readIdx] = -1;
      }
      writeIdx++;
    }
  }
  
  log(`defragment: compacted used blocks to front (used=${writeIdx})`);
  saveState();
  return { defragmented: true };
}

function markBadBlocks(count) {
  let marked = 0;
  for (let tries = 0; tries < diskState.blocks * 4 && marked < count; tries++) {
    const idx = Math.floor(Math.random() * diskState.blocks);
    if (diskState.state[idx] === 0) { // free
      diskState.state[idx] = 2; // bad
      diskState.owner[idx] = -1;
      marked++;
    }
  }
  
  log(`mark_bad: requested=${count} marked=${marked}`);
  saveState();
  return { marked: marked > 0 };
}

function repair() {
  let repaired = 0;
  for (let i = 0; i < diskState.blocks; i++) {
    if (diskState.state[i] === 2) { // bad
      if (Math.random() < 0.5) { // 50% chance to repair
        diskState.state[i] = 0; // free
        repaired++;
      }
    }
  }
  
  log(`repair: repaired=${repaired} bad->free`);
  saveState();
  return { repaired };
}

function getStats() {
  const total = diskState.blocks;
  const used = diskState.state.filter(s => s === 1).length;
  const free = diskState.state.filter(s => s === 0).length;
  const badSectors = diskState.state.filter(s => s === 2).length;
  
  // Calculate fragmentation
  let fragmentedFiles = 0;
  let totalFiles = 0;
  
  for (const fileId in diskState.files) {
    if (diskState.files[fileId].status === 'active') {
      totalFiles++;
      const blocks = [];
      for (let i = 0; i < diskState.blocks; i++) {
        if (diskState.owner[i] == fileId && diskState.state[i] === 1) {
          blocks.push(i);
        }
      }
      
      // Check if fragmented
      let segments = 0;
      let inRun = false;
      for (let i = 0; i < diskState.blocks; i++) {
        if (diskState.owner[i] == fileId && diskState.state[i] === 1) {
          if (!inRun) {
            inRun = true;
            segments++;
          }
        } else {
          inRun = false;
        }
      }
      
      if (segments > 1) fragmentedFiles++;
    }
  }
  
  const fragmentationPercent = totalFiles > 0 ? (fragmentedFiles / totalFiles) * 100 : 0;
  
  return {
    total,
    used,
    free,
    badSectors,
    fragmentationPercent: Math.round(fragmentationPercent * 100) / 100
  };
}

function getDiskState() {
  const blocks = [];
  for (let i = 0; i < diskState.blocks; i++) {
    blocks.push({
      index: i,
      state: diskState.state[i] === 0 ? 'free' : diskState.state[i] === 1 ? 'used' : 'bad',
      fileId: diskState.owner[i] > 0 && diskState.state[i] === 1 ? diskState.owner[i] : null
    });
  }
  return { blocks };
}

function getFiles() {
  const files = [];
  for (const fileId in diskState.files) {
    if (diskState.files[fileId].status === 'active') {
      const size = diskState.state.filter((s, i) => diskState.owner[i] == fileId && s === 1).length;
      files.push({
        id: parseInt(fileId),
        status: 'active',
        size
      });
    }
  }
  return { files };
}

function reset() {
  diskState = {
    blocks: 1024,
    state: new Array(1024).fill(0),
    owner: new Array(1024).fill(-1),
    files: {},
    nextFileId: 1,
    logs: []
  };
  saveState();
  log('disk_reset: disk reinitialized');
  return { reset: true };
}

// System disk information endpoint - REAL SYSTEM DISK INFO
function getSystemDiskInfo() {
  try {
    // Try to get actual disk space using Node.js fs module
    const fs = require('fs');
    
    // For Windows, we can use a different approach to get disk space
    if (os.platform() === 'win32') {
      try {
        // Use fs.statfs to get disk space (available in newer Node.js versions)
        const stats = fs.statfsSync('C:\\');
        const totalBytes = stats.bsize * stats.blocks;
        const freeBytes = stats.bsize * stats.bfree;
        const usedBytes = totalBytes - freeBytes;
        
        const totalGB = Math.round(totalBytes / (1024 * 1024 * 1024));
        const freeGB = Math.round(freeBytes / (1024 * 1024 * 1024));
        const usedGB = Math.round(usedBytes / (1024 * 1024 * 1024));
        const usedPercentage = Math.round((usedBytes / totalBytes) * 100);
        const freePercentage = Math.round((freeBytes / totalBytes) * 100);
        
        return {
          total: totalGB,
          free: freeGB,
          used: usedGB,
          usedPercentage: usedPercentage,
          freePercentage: freePercentage,
          badSectors: 0, // Hard to get without admin privileges
          path: 'C:\\'
        };
      } catch (fsError) {
        // Fallback if fs.statfs doesn't work
        console.log('fs.statfs failed, falling back to disk info');
      }
    }
    
    // Fallback to basic OS info if platform-specific methods fail
    const disks = os.totalmem();
    const freeMem = os.freemem();
    const usedMem = disks - freeMem;
    
    return {
      total: Math.round(disks / (1024 * 1024 * 1024)), // GB
      free: Math.round(freeMem / (1024 * 1024 * 1024)), // GB
      used: Math.round(usedMem / (1024 * 1024 * 1024)), // GB
      usedPercentage: Math.round((usedMem / disks) * 100),
      freePercentage: Math.round((freeMem / disks) * 100),
      badSectors: 0, // Hard to get without admin privileges
      path: os.platform() === 'win32' ? 'C:\\' : '/'
    };
  } catch (error) {
    // Fallback to simulation if there's an error
    return {
      total: 512, // GB
      free: 256, // GB
      used: 256, // GB
      usedPercentage: 50,
      freePercentage: 50,
      badSectors: 0,
      path: os.platform() === 'win32' ? 'C:\\' : '/'
    };
  }
}

// Create a real file on the system
function createRealFile(filename, size) {
  try {
    // Create file in the current directory or a specific directory
    const filePath = path.join(__dirname, filename);
    
    // Check if file already exists
    if (fs.existsSync(filePath)) {
      throw new Error(`File ${filename} already exists`);
    }
    
    // Create a buffer of the specified size
    const buffer = Buffer.alloc(size || 1024, 0); // Default to 1KB if no size specified
    
    // Write the file
    fs.writeFileSync(filePath, buffer);
    
    log(`Created real file: ${filename} (${size || 1024} bytes)`);
    return { success: true, message: `File ${filename} created successfully` };
  } catch (error) {
    log(`Failed to create file: ${filename} - ${error.message}`);
    throw new Error(`Failed to create file: ${error.message}`);
  }
}

// Delete a real file from the system
function deleteRealFile(filename) {
  try {
    // Delete file from the current directory or a specific directory
    const filePath = path.join(__dirname, filename);
    
    // Check if file exists
    if (!fs.existsSync(filePath)) {
      throw new Error(`File ${filename} does not exist`);
    }
    
    // Delete the file
    fs.unlinkSync(filePath);
    
    log(`Deleted real file: ${filename}`);
    return { success: true, message: `File ${filename} deleted successfully` };
  } catch (error) {
    log(`Failed to delete file: ${filename} - ${error.message}`);
    throw new Error(`Failed to delete file: ${error.message}`);
  }
}

// HTTP Server
const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const method = req.method;
  const path = url.pathname;
  
  // CORS headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, DELETE, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  
  if (method === 'OPTIONS') {
    res.writeHead(200);
    res.end();
    return;
  }
  
  let body = '';
  req.on('data', chunk => {
    body += chunk.toString();
  });
  
  req.on('end', () => {
    try {
      let result;
      
      // Route handling
      if (method === 'GET' && path === '/disk/state') {
        result = getDiskState();
      } else if (method === 'GET' && path === '/disk/stats') {
        result = getStats();
      } else if (method === 'GET' && path === '/disk/files') {
        result = getFiles();
      } else if (method === 'GET' && path === '/disk/logs') {
        result = { logs: diskState.logs };
      } else if (method === 'GET' && path === '/system-disk') {
        result = getSystemDiskInfo();
      } else if (method === 'POST' && path === '/allocate/contiguous') {
        const data = JSON.parse(body);
        result = allocateContiguous(data.size);
      } else if (method === 'POST' && path === '/allocate/fragmented') {
        const data = JSON.parse(body);
        result = allocateFragmented(data.size);
      } else if (method === 'POST' && path === '/allocate/custom') {
        const data = JSON.parse(body);
        result = allocateCustom(data.size, data.strategy || 'first-fit');
      } else if (method === 'POST' && path === '/defragment') {
        result = defragment();
      } else if (method === 'POST' && path === '/mark-bad') {
        const data = JSON.parse(body);
        result = markBadBlocks(data.count);
      } else if (method === 'POST' && path === '/repair') {
        result = repair();
      } else if (method === 'POST' && path === '/disk/reset') {
        result = reset();
      } else if (method === 'POST' && path === '/create-file') {
        // Create a real file on the system
        const data = JSON.parse(body);
        result = createRealFile(data.filename, data.size);
      } else if (method === 'POST' && path === '/delete-file') {
        // Delete a real file from the system
        const data = JSON.parse(body);
        result = deleteRealFile(data.filename);
      } else {
        res.writeHead(404);
        res.end(JSON.stringify({ success: false, error: 'Endpoint not found' }));
        return;
      }
      
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: true, data: result }));
      
    } catch (error) {
      res.writeHead(500, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, error: error.message }));
    }
  });
});

server.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}/`);
});