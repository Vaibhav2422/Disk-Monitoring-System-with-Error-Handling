"use client"

import useSWR from "swr"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Progress } from "@/components/ui/progress"
import { fetcher } from "@/lib/api"

type SystemDiskInfo = {
  total: number
  free: number
  used: number
  usedPercentage: number
  freePercentage: number
  badSectors: number
  path: string
}

export function SystemDiskInfo() {
  const { data, isLoading, error, mutate } = useSWR<SystemDiskInfo>("/system-disk", fetcher, {
    refreshInterval: 30000, // Refresh every 30 seconds
    revalidateOnFocus: false,
  })

  if (isLoading) {
    return (
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-lg">System Disk Information</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-muted-foreground">Loading disk information...</div>
        </CardContent>
      </Card>
    )
  }

  if (error) {
    return (
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-lg">System Disk Information</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-red-500">Failed to load disk information</div>
          <button 
            onClick={() => mutate()}
            className="mt-2 px-3 py-1 bg-gray-200 rounded text-sm"
          >
            Retry
          </button>
        </CardContent>
      </Card>
    )
  }

  if (!data) {
    return (
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-lg">System Disk Information</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="text-sm text-muted-foreground">No disk information available</div>
        </CardContent>
      </Card>
    )
  }

  return (
    <Card>
      <CardHeader className="pb-2">
        <CardTitle className="text-lg">System Disk Information</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        <div className="space-y-1">
          <div className="flex justify-between text-sm">
            <span>Disk Usage</span>
            <span className="font-medium">{data.usedPercentage}%</span>
          </div>
          <Progress value={data.usedPercentage} className="h-2" />
        </div>
        
        <div className="grid grid-cols-2 gap-4">
          <div className="space-y-1">
            <div className="text-xs text-muted-foreground">Total Space</div>
            <div className="text-lg font-medium">{data.total} GB</div>
          </div>
          <div className="space-y-1">
            <div className="text-xs text-muted-foreground">Free Space</div>
            <div className="text-lg font-medium">{data.free} GB</div>
          </div>
          <div className="space-y-1">
            <div className="text-xs text-muted-foreground">Used Space</div>
            <div className="text-lg font-medium">{data.used} GB</div>
          </div>
          <div className="space-y-1">
            <div className="text-xs text-muted-foreground">Bad Sectors</div>
            <div className="text-lg font-medium">{data.badSectors}</div>
          </div>
        </div>
        
        <div className="text-xs text-muted-foreground">
          Path: {data.path}
        </div>
      </CardContent>
    </Card>
  )
}