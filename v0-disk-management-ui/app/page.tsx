import { ControlsPanel } from "@/components/controls-panel"
import { DiskGrid } from "@/components/disk-grid"
import { FilesTable } from "@/components/files-table"
import { StatsCard } from "@/components/stats-card"
import { LogsList } from "@/components/logs-list"
import { SystemDiskInfo } from "@/components/system-disk-info"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"

export default function Page() {
  return (
    <main className="min-h-dvh p-4 md:p-6 lg:p-8">
      <header className="mb-6 md:mb-8">
        <h1 className="text-balance text-2xl md:text-3xl font-semibold">Disk Management Simulator</h1>
        <p className="text-muted-foreground mt-1">
          Manage allocations, visualize the disk, and inspect stats and logs.
        </p>
      </header>

      <section className="grid grid-cols-1 gap-6 lg:grid-cols-3">
        <div className="lg:col-span-2 space-y-6">
          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">Controls</CardTitle>
            </CardHeader>
            <CardContent>
              <ControlsPanel />
            </CardContent>
          </Card>

          {/* Move Files table above Disk Visualization */}
          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">Files</CardTitle>
            </CardHeader>
            <CardContent className="overflow-auto">
              <FilesTable />
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">Disk Visualization</CardTitle>
            </CardHeader>
            <CardContent>
              <DiskGrid />
            </CardContent>
          </Card>
        </div>

        <div className="space-y-6">
          <SystemDiskInfo />
          <StatsCard />
          <Card>
            <CardHeader>
              <CardTitle className="text-pretty">Logs</CardTitle>
            </CardHeader>
            <CardContent className="max-h-[420px] overflow-auto">
              <LogsList />
            </CardContent>
          </Card>
        </div>
      </section>
    </main>
  )
}