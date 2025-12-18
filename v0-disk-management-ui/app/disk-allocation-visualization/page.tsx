"use client"

import { useState, useEffect } from "react"
import useSWR from "swr"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { fetcher } from "@/lib/api"
import { cn } from "@/lib/utils"

// Types for our disk data
type DiskBlock = {
  index: number
  state: "free" | "used" | "bad"
  fileId: number | null
}

type DiskFile = {
  id: number
  status: string
  size: number
}

type DiskData = {
  blocks: DiskBlock[]
}

type FilesData = {
  files: DiskFile[]
}

// Helper function to determine if a file is fragmented
const isFileFragmented = (fileId: number, blocks: DiskBlock[]): boolean => {
  const fileBlocks = blocks
    .map((block, index) => ({ ...block, index }))
    .filter(block => block.fileId === fileId && block.state === "used")
    .sort((a, b) => a.index - b.index)
  
  if (fileBlocks.length <= 1) return false
  
  // Check if blocks are contiguous
  for (let i = 1; i < fileBlocks.length; i++) {
    if (fileBlocks[i].index !== fileBlocks[i-1].index + 1) {
      return true
    }
  }
  return false
}

// Color palette for file IDs
const getFileColor = (fileId: number): string => {
  const colors = [
    "bg-blue-500",
    "bg-green-500",
    "bg-yellow-500",
    "bg-purple-500",
    "bg-pink-500",
    "bg-indigo-500",
    "bg-red-500",
    "bg-teal-500",
    "bg-orange-500",
    "bg-cyan-500",
  ]
  return colors[fileId % colors.length]
}

export default function DiskAllocationVisualizationPage() {
  const { data: diskData, isLoading: diskLoading, mutate: mutateDisk } = useSWR<DiskData>("/disk/state", fetcher)
  const { data: filesData, isLoading: filesLoading } = useSWR<FilesData>("/disk/files", fetcher)
  const [gridSize, setGridSize] = useState(32) // Default to 32x32 grid
  const [showFragmented, setShowFragmented] = useState(true)
  const [showDefragmented, setShowDefragmented] = useState(true)

  // Calculate grid dimensions
  const blocks = diskData?.blocks || []
  const totalBlocks = blocks.length
  const rows = Math.ceil(totalBlocks / gridSize)

  // Get unique file IDs for legend
  const fileIds = [...new Set(blocks.map(block => block.fileId).filter(id => id !== null))] as number[]

  // Filter files based on current data
  const files = filesData?.files || []

  return (
    <main className="min-h-dvh p-4 md:p-6 lg:p-8">
      <header className="mb-6 md:mb-8">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-balance text-2xl md:text-3xl font-semibold">Disk Allocation Visualization</h1>
            <p className="text-muted-foreground mt-1">
              Visualize how files are allocated on the disk with color-coded blocks
            </p>
          </div>
          <Button onClick={() => window.history.back()}>
            Back to Dashboard
          </Button>
        </div>
      </header>

      <section className="grid grid-cols-1 lg:grid-cols-4 gap-6">
        <div className="lg:col-span-3 space-y-6">
          <Card>
            <CardHeader>
              <div className="flex flex-wrap items-center justify-between gap-4">
                <CardTitle className="text-pretty">Disk Allocation Map</CardTitle>
                <div className="flex flex-wrap gap-2">
                  <Button 
                    variant="outline" 
                    size="sm" 
                    onClick={() => mutateDisk()}
                  >
                    Refresh
                  </Button>
                  <div className="flex items-center gap-2 text-sm">
                    <label className="flex items-center gap-1">
                      <input
                        type="checkbox"
                        checked={showFragmented}
                        onChange={(e) => setShowFragmented(e.target.checked)}
                        className="rounded"
                      />
                      Fragmented
                    </label>
                    <label className="flex items-center gap-1">
                      <input
                        type="checkbox"
                        checked={showDefragmented}
                        onChange={(e) => setShowDefragmented(e.target.checked)}
                        className="rounded"
                      />
                      Defragmented
                    </label>
                  </div>
                </div>
              </div>
            </CardHeader>
            <CardContent>
              <div className="space-y-4">
                <div className="text-sm text-muted-foreground">
                  {diskLoading ? "Loading..." : `${totalBlocks} blocks · grid ${gridSize}×${rows}`}
                </div>
                
                <div
                  className="grid gap-[2px] border rounded-md p-2"
                  style={{
                    gridTemplateColumns: `repeat(${gridSize}, minmax(0, 1fr))`,
                    backgroundColor: "var(--color-border)",
                  }}
                  aria-label="Disk Allocation Grid"
                  role="grid"
                >
                  {blocks.map((block, index) => {
                    // Determine block appearance
                    let blockClass = "w-4 h-4 md:w-5 md:h-5 lg:w-6 lg:h-6 rounded-sm border border-border"
                    
                    if (block.state === "free") {
                      blockClass += " bg-white dark:bg-gray-800"
                    } else if (block.state === "bad") {
                      blockClass += " bg-red-500"
                    } else if (block.state === "used" && block.fileId !== null) {
                      const isFragmented = isFileFragmented(block.fileId, blocks)
                      
                      // Show block based on filter settings
                      if ((isFragmented && showFragmented) || (!isFragmented && showDefragmented)) {
                        blockClass += ` ${getFileColor(block.fileId)}`
                        
                        // Add border to distinguish fragmented vs defragmented
                        if (isFragmented) {
                          blockClass += " ring-2 ring-yellow-400"
                        }
                      } else {
                        // Hidden by filter
                        blockClass += " bg-white dark:bg-gray-800 opacity-50"
                      }
                    } else {
                      blockClass += " bg-white dark:bg-gray-800"
                    }
                    
                    return (
                      <div
                        key={index}
                        role="gridcell"
                        aria-label={`Block ${index + 1} - ${block.state}${block.fileId ? ` (File ${block.fileId})` : ""}`}
                        className={blockClass}
                        title={`Block #${index + 1} - ${block.state}${block.fileId ? ` (File ${block.fileId})` : ""}${block.state === "used" && block.fileId !== null ? ` (${isFileFragmented(block.fileId, blocks) ? "Fragmented" : "Defragmented"})` : ""}`}
                      />
                    )
                  })}
                </div>
                
                <div className="flex flex-wrap items-center gap-4 text-sm">
                  <div className="flex items-center gap-2">
                    <span className="inline-flex items-center gap-1">
                      <span className="w-4 h-4 rounded-sm bg-white dark:bg-gray-800 border border-border" />
                      Free
                    </span>
                    <span className="inline-flex items-center gap-1">
                      <span className="w-4 h-4 rounded-sm bg-red-500 border border-border" />
                      Bad Block
                    </span>
                  </div>
                  
                  <div className="flex items-center gap-2">
                    <span className="inline-flex items-center gap-1">
                      <span className="w-4 h-4 rounded-sm bg-blue-500 border border-border ring-2 ring-yellow-400" />
                      Fragmented File
                    </span>
                    <span className="inline-flex items-center gap-1">
                      <span className="w-4 h-4 rounded-sm bg-blue-500 border border-border" />
                      Defragmented File
                    </span>
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
        
        <div className="space-y-6">
          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">Legend</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-3">
                <h3 className="font-medium">Files</h3>
                <div className="space-y-2 max-h-60 overflow-auto">
                  {fileIds.length === 0 ? (
                    <p className="text-sm text-muted-foreground">No files allocated</p>
                  ) : (
                    fileIds.map(fileId => {
                      const file = files.find(f => f.id === fileId)
                      const isFragmented = diskData ? isFileFragmented(fileId, diskData.blocks) : false
                      return (
                        <div key={fileId} className="flex items-center justify-between">
                          <div className="flex items-center gap-2">
                            <span className={cn("w-4 h-4 rounded-sm", getFileColor(fileId))} />
                            <span>File {fileId}</span>
                          </div>
                          <span className={cn(
                            "text-xs px-2 py-1 rounded",
                            isFragmented 
                              ? "bg-yellow-100 text-yellow-800 dark:bg-yellow-900 dark:text-yellow-100" 
                              : "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-100"
                          )}>
                            {isFragmented ? "Fragmented" : "Defragmented"}
                          </span>
                        </div>
                      )
                    })
                  )}
                </div>
              </div>
              
              <div className="mt-4 pt-4 border-t">
                <h3 className="font-medium mb-2">Display Options</h3>
                <div className="space-y-3">
                  <div>
                    <label className="text-sm font-medium">Grid Size</label>
                    <div className="flex gap-2 mt-1">
                      {[16, 32, 64].map(size => (
                        <Button
                          key={size}
                          variant={gridSize === size ? "default" : "outline"}
                          size="sm"
                          onClick={() => setGridSize(size)}
                          className="text-xs"
                        >
                          {size}×{size}
                        </Button>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">About This Visualization</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="text-sm space-y-2">
                <p>
                  This visualization shows how files are allocated on the disk:
                </p>
                <ul className="list-disc pl-5 space-y-1">
                  <li>Each block represents 1 KB of storage</li>
                  <li>Different colors represent different files</li>
                  <li>Yellow borders indicate fragmented files</li>
                  <li>Solid colors indicate defragmented (contiguous) files</li>
                </ul>
                <p className="pt-2">
                  Use the checkboxes to filter which types of allocations are shown, 
                  and adjust the grid size for different views.
                </p>
              </div>
            </CardContent>
          </Card>
        </div>
      </section>
    </main>
  )
}