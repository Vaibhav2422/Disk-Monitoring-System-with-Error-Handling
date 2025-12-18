"use client"

import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { useToast } from "@/hooks/use-toast"
import { postJSON } from "@/lib/api"
import { useState } from "react"
import { useRouter } from "next/navigation"

type AllocationStrategy = "first-fit" | "best-fit" | "worst-fit" | "contiguous" | "fragmented"

export function ControlsPanel() {
  const { toast } = useToast()
  const router = useRouter()
  const [fileSize, setFileSize] = useState("")
  const [allocationStrategy, setAllocationStrategy] = useState<AllocationStrategy>("first-fit")
  const [fileName, setFileName] = useState("")

  const handleAllocate = async () => {
    if (!fileSize) {
      toast({
        title: "Error",
        description: "Please enter a file size",
        variant: "destructive",
      })
      return
    }

    const size = parseInt(fileSize)
    if (isNaN(size) || size <= 0) {
      toast({
        title: "Error",
        description: "Please enter a valid positive number for file size",
        variant: "destructive",
      })
      return
    }

    try {
      let response
      switch (allocationStrategy) {
        case "contiguous":
          response = await postJSON("/allocate/contiguous", { size })
          break
        case "fragmented":
          response = await postJSON("/allocate/fragmented", { size })
          break
        default:
          response = await postJSON("/allocate/custom", { size, strategy: allocationStrategy })
      }

      if (response.ok) {
        toast({
          title: "Success",
          description: "File allocated successfully",
        })
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to allocate file",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to allocate file",
        variant: "destructive",
      })
    }
  }

  const handleDefragment = async () => {
    try {
      const response = await postJSON("/defragment", {})
      if (response.ok) {
        toast({
          title: "Success",
          description: "Disk defragmented successfully",
        })
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to defragment disk",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to defragment disk",
        variant: "destructive",
      })
    }
  }

  const handleMarkBad = async () => {
    try {
      const response = await postJSON("/mark-bad", { count: 5 })
      if (response.ok) {
        toast({
          title: "Success",
          description: "Bad blocks marked successfully",
        })
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to mark bad blocks",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to mark bad blocks",
        variant: "destructive",
      })
    }
  }

  const handleRepair = async () => {
    try {
      const response = await postJSON("/repair", {})
      if (response.ok) {
        toast({
          title: "Success",
          description: "Disk repaired successfully",
        })
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to repair disk",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to repair disk",
        variant: "destructive",
      })
    }
  }

  const handleReset = async () => {
    try {
      const response = await postJSON("/disk/reset", {})
      if (response.ok) {
        toast({
          title: "Success",
          description: "Disk reset successfully",
        })
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to reset disk",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to reset disk",
        variant: "destructive",
      })
    }
  }

  // New functions for real file operations
  const handleCreateRealFile = async () => {
    if (!fileName) {
      toast({
        title: "Error",
        description: "Please enter a file name",
        variant: "destructive",
      })
      return
    }

    const size = fileSize ? parseInt(fileSize) : 1024 // Default to 1KB if no size specified
    if (isNaN(size) || size <= 0) {
      toast({
        title: "Error",
        description: "Please enter a valid positive number for file size",
        variant: "destructive",
      })
      return
    }

    try {
      const response = await postJSON("/create-file", { filename: fileName, size })
      if (response.ok) {
        const data = await response.json()
        toast({
          title: "Success",
          description: data.data.message || "File created successfully",
        })
        // Clear the input fields
        setFileName("")
        setFileSize("")
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to create file",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to create file",
        variant: "destructive",
      })
    }
  }

  const handleDeleteRealFile = async () => {
    if (!fileName) {
      toast({
        title: "Error",
        description: "Please enter a file name",
        variant: "destructive",
      })
      return
    }

    try {
      const response = await postJSON("/delete-file", { filename: fileName })
      if (response.ok) {
        const data = await response.json()
        toast({
          title: "Success",
          description: data.data.message || "File deleted successfully",
        })
        // Clear the input field
        setFileName("")
      } else {
        const error = await response.json()
        toast({
          title: "Error",
          description: error.error || "Failed to delete file",
          variant: "destructive",
        })
      }
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to delete file",
        variant: "destructive",
      })
    }
  }

  // Navigate to the disk allocation visualization page
  const handleViewVisualization = () => {
    router.push("/disk-allocation-visualization")
  }

  return (
    <div className="space-y-6">
      <div className="space-y-4">
        <h3 className="text-lg font-medium">Disk Allocation</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="space-y-2">
            <Label htmlFor="file-size">File Size (blocks)</Label>
            <Input
              id="file-size"
              type="number"
              value={fileSize}
              onChange={(e) => setFileSize(e.target.value)}
              placeholder="Enter file size"
            />
          </div>
          <div className="space-y-2">
            <div className="flex items-end justify-between">
              <Label htmlFor="allocation-strategy">Allocation Strategy</Label>
              <Button 
                onClick={handleViewVisualization} 
                variant="outline" 
                size="sm"
                className="h-8 px-2 text-xs"
              >
                View Visualization
              </Button>
            </div>
            <Select value={allocationStrategy} onValueChange={(value: AllocationStrategy) => setAllocationStrategy(value)}>
              <SelectTrigger id="allocation-strategy">
                <SelectValue placeholder="Select strategy" />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="first-fit">First Fit</SelectItem>
                <SelectItem value="best-fit">Best Fit</SelectItem>
                <SelectItem value="worst-fit">Worst Fit</SelectItem>
                <SelectItem value="contiguous">Contiguous</SelectItem>
                <SelectItem value="fragmented">Fragmented</SelectItem>
              </SelectContent>
            </Select>
          </div>
        </div>
        <Button onClick={handleAllocate} className="w-full">
          Allocate File
        </Button>
      </div>

      <div className="space-y-4">
        <h3 className="text-lg font-medium">Real File Operations</h3>
        <div className="space-y-2">
          <Label htmlFor="file-name">File Name</Label>
          <Input
            id="file-name"
            type="text"
            value={fileName}
            onChange={(e) => setFileName(e.target.value)}
            placeholder="Enter file name"
          />
        </div>
        <div className="grid grid-cols-2 gap-2">
          <Button onClick={handleCreateRealFile} variant="secondary">
            Create File
          </Button>
          <Button onClick={handleDeleteRealFile} variant="destructive">
            Delete File
          </Button>
        </div>
      </div>

      <div className="space-y-4">
        <h3 className="text-lg font-medium">Disk Management</h3>
        <div className="grid grid-cols-2 gap-2">
          <Button onClick={handleDefragment} variant="outline">
            Defragment
          </Button>
          <Button onClick={handleMarkBad} variant="outline">
            Mark Bad Blocks
          </Button>
          <Button onClick={handleRepair} variant="outline">
            Repair
          </Button>
          <Button onClick={handleReset} variant="outline">
            Reset Disk
          </Button>
        </div>
      </div>
    </div>
  )
}