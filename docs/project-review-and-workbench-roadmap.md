# MB-Studio Project Review and Workbench Roadmap

Review date: October 14, 2026

Scope: the current MB-Studio application under `src/mbstudio`, its integration
with MB-System, and the desktop workflow described by the user.

This document records findings from the source review and proposes a development
direction. Proposed interfaces, interactions, and acceptance criteria are future
work unless explicitly identified as implemented or tested.

## 1. Overall Assessment

MB-Studio has a useful foundation for project management, background processing,
typed MB-System command generation, and basic visualization. The next development
stage should connect these capabilities into a professional desktop workflow:

**Inspect data -> edit interactively -> process -> validate results -> deliver.**

Two additional product requirements apply to the roadmap:

- Use GMT for scientific cartography, figure composition, and publication/export
  output, alongside the workbench's interactive data views and 3D renderer.
- Use an application-drawn custom title bar. The operating system's standard
  title bar must not be used for MB-Studio-owned windows.

The user's concerns are supported by the current implementation:

- The shell has panels and tabs, but its fixed layout and shared view models
  limit document handling and comparison.
- Several MB-System processing operations have parameter panels, while essential
  interactive workflows remain missing.
- The active Grid view displays a shaded 2D image. It does not implement a 3D
  scene or sounding editor.
- Processing state does not adequately communicate pending edits, unapplied
  corrections, outdated processed files, or products that need rebuilding.
- Task execution and result inspection have correctness and traceability issues
  that should be addressed before broader feature expansion.

Retain the kernel, project services, background execution, and typed processing
recipes. Focus changes on task coordination, document ownership, data versions,
the workbench shell, and interactive editing services.

## 2. Review Basis and Verification Limits

The review inspected the QML shell, view models, project service, MB-System
adapter services, build configuration, and existing test sources.

A temporary C++ probe compiled the repository's actual `JobService.cpp` and the
required kernel/platform sources. It verified that:

1. Two jobs declaring identical input and output paths can execute concurrently.
2. Retrying a failed job reuses its working directory and output paths.
3. A successful retry can replace the output written by the failed attempt.

These checks used in-process job actions and temporary files. They did not run
MB-System programs or demonstrate corruption of survey data. The project-history
consequences of retry were identified by tracing the source code.

The full project test suite and actual GUI interactions were not run during this
review. Visual findings are based on layout and control definitions, rather than
an interactive inspection of the installed application. Performance and platform
compatibility of proposed features have not been established.

## 3. Prioritized Findings

P1 findings affect result correctness, consistency, or traceability. P2 findings
affect workbench behavior, workflow clarity, or usability. The missing 3D and
editing capabilities are identified separately as a core product gap.

### F1. P1: Concurrent Jobs Can Receive the Same Output Path

**Evidence.** `ProjectService::uniqueRootLocked()` checks whether the candidate
file already exists. It does not reserve paths assigned to queued or running
jobs. `JobService` runs jobs concurrently without using their declared inputs
and outputs to prevent conflicting access.

**Trigger.** Submit two grid jobs with the same product name before the first
job creates its output file. Both plans can select the same path. Operations
that modify the same survey line also have no scheduler-level resource
exclusion.

**Impact.** Jobs can overwrite shared products or operate on changing inputs.
MB-System tools may provide some protection of their own, but the application
does not establish a consistent policy.

**Recommendation.** Reserve output paths when submitting jobs. Define read/write
access to survey lines, edit files, parameter files, and products. Serialize
conflicting operations or express dependencies between them.

Sources: [ProjectService.cpp, output naming](../src/mbstudio/plugins/project/ProjectService.cpp#L1069),
[JobService.cpp, scheduling](../src/mbstudio/plugins/mbsystem/JobService.cpp#L148).

### F2. P1: Failed-Job Retry Bypasses Project History Registration

**Evidence.** `JobService::retry()` copies the previous `JobSpec` and enqueues it
with a new job ID. The working directory, output paths, and captured result
registration action are retained. This path does not call
`ProjectService::run()`, which creates the project history entry and populates
the pending-job mapping.

**Trigger.** Retry a failed or cancelled project job from the Jobs panel.

**Impact.** The new attempt can complete without receiving its own project
history entry. The original record can remain failed, while result registration
still refers to the original project sequence. Output files are also reused.

**Recommendation.** Route project retries through a project-level operation that
creates a new attempt record, assigns outputs deliberately, and preserves the
relationship to the previous attempt. Keep generic service-level retry available
for jobs whose owners explicitly support those semantics.

Sources: [JobListModel.cpp, retry entry](../src/mbstudio/plugins/qmlui/viewmodels/JobListModel.cpp#L175),
[JobService.cpp, copied specification](../src/mbstudio/plugins/mbsystem/JobService.cpp#L324),
[ProjectService.cpp, history synchronization](../src/mbstudio/plugins/project/ProjectService.cpp#L1384).

### F3. P1: Inspecting a Processed Line Still Opens the Original File

**Evidence.** `AppController::inspectLine()` reads the `path` and `format` exposed
by `ProjectViewModel`. Those values describe the original file, even when the
project has a preprocessing path or an `mbprocess` result.

**Trigger.** Clean or correct a line, run processing, and then inspect that line
through the project interface.

**Impact.** The inspector can present original beam statistics and depths when
the user expects to assess the processed result.

**Recommendation.** Make the inspected data version explicit: Original,
Preprocessed, or Processed. Resolve its path and format through the project
service. Show the active version in each document and support before/after
comparison.

Sources: [AppController.cpp, line inspection](../src/mbstudio/plugins/qmlui/viewmodels/AppController.cpp#L321),
[ProjectViewModel.cpp, original file metadata](../src/mbstudio/plugins/qmlui/viewmodels/ProjectViewModel.cpp#L195).

### F4. P1: Projected Grid Generation and Map Display Are Disconnected

**Evidence.** Processing panels offer UTM output, but the map-layer loading path
rejects grids whose `geographic` flag is false. The job completion handler can
still announce that the result is on the map.

**Trigger.** Generate a projected grid and attempt to inspect it alongside survey
tracks on the project map.

**Impact.** A valid processing choice produces an incomplete viewing workflow,
and completion feedback can misrepresent what is visible.

**Recommendation.** Preserve a complete coordinate reference description and
define map display transformation. Until projected layers are supported, explain
the limitation before execution and open the generated grid in a supported
viewer. Completion feedback must reflect the actual display outcome.

Sources: [ProcessingViewModel.cpp, coordinate options](../src/mbstudio/plugins/qmlui/viewmodels/ProcessingViewModel.cpp#L355),
[ProjectViewModel.cpp, projected-grid rejection](../src/mbstudio/plugins/qmlui/viewmodels/ProjectViewModel.cpp#L753),
[AppController.cpp, completion feedback](../src/mbstudio/plugins/qmlui/viewmodels/AppController.cpp#L349).

### F5. Core Product Gap: No Integrated 3D Scene or Sounding Editor

**Evidence.** `GridPage.qml` displays `grid.imageSource` in an `Image`, with 2D
pan and zoom. Vertical exaggeration influences hillshade rendering. The active
swath service returns a summary and decimated navigation, without a per-sounding
data contract or editing interface.

**Impact.** The current application cannot provide scene rotation, 3D selection,
cross-section inspection, or manual sounding flagging. Exporting a 3D model does
not provide those interactions within the workbench.

**Recommendation.** Implement terrain viewing and sounding editing as connected
but distinct capabilities. Add data access, scene rendering, selection, editing,
and persistence with explicit ownership.

This finding concerns the active MB-Studio frontend. The repository also
contains legacy viewers and experimental Qt tools, but those are separate build
paths and are not integrated into this workbench. Audit their dependencies and
data ownership before reusing components.

Sources: [GridPage.qml, image viewport](../src/mbstudio/plugins/qmlui/qml/pages/GridPage.qml#L56),
[SwathService.h, available data](../src/mbstudio/kernel/include/mbstudio/api/SwathService.h#L18),
[CMakeLists.txt, separate GUI build paths](../CMakeLists.txt#L328).

### F6. P2: Document Tabs Share View Instances

**Evidence.** Built-in workspace contributions are registered as singletons.
`AppController` owns one grid view model and one swath view model. Opening another
file replaces the data associated with that view type. Restored tab arguments
are not passed to the QML document loaders to reconstruct independent views.

**Impact.** Users cannot keep two grids or survey lines open for independent
inspection and comparison. Restoring tab records does not fully restore their
data and viewing state.

**Recommendation.** Separate a document from its view type. Each document needs
an identity, data reference, data version, owned view model, and saved view state.
Allow multiple documents and split views. Restore referenced data as well as tab
labels and ordering.

Sources: [QmlFrontend.cpp, singleton registration](../src/mbstudio/plugins/qmlui/QmlFrontend.cpp#L34),
[AppController.cpp, shared view models](../src/mbstudio/plugins/qmlui/viewmodels/AppController.cpp#L70),
[Main.qml, document loaders](../src/mbstudio/plugins/qmlui/qml/Main.qml#L365),
[WorkspaceModel.cpp, saved tab state](../src/mbstudio/plugins/qmlui/viewmodels/WorkspaceModel.cpp#L165).

### F7. P2: Fixed Panels and Duplicate Sidebars Constrain the Main View

**Evidence.** The shell uses fixed `SplitView` regions, with remembered sizes and
visibility. It does not provide panel docking, tab grouping, floating panels, or
document splitting. The Grid page adds a 300-pixel sidebar inside a shell that
already has a right properties/processing panel.

**Impact.** At the default 1440-pixel window width, the default left and right
panels and the Grid page's own sidebar leave approximately 400 pixels for the
grid viewport, before accounting for all separators. Material controls, rounded
cards, and page-level padding further reduce the space available for data.

**Recommendation.** Consolidate object properties and display settings in shared
context-sensitive panels. Introduce a docking layout and document splits. Use
compact desktop controls, restrained spacing, and a neutral visual hierarchy.

Sources: [Main.qml, workbench layout](../src/mbstudio/plugins/qmlui/qml/Main.qml#L277),
[GridPage.qml, internal sidebar](../src/mbstudio/plugins/qmlui/qml/pages/GridPage.qml#L149),
[Theme.qml, visual tokens](../src/mbstudio/plugins/qmlui/qml/Theme.qml#L8).

### F8. P2: Processing State Does Not Explain What Needs Updating

**Evidence.** Survey line state primarily distinguishes `raw`, `preprocessed`,
and `processed`. The data model does not adequately represent pending cleaning
edits, unapplied correction changes, outdated processed data, or downstream
products that need rebuilding.

**Impact.** Users must remember which steps they changed and decide manually
whether the current product reflects those changes.

**Recommendation.** Track source revisions, edit revisions, correction
revisions, processed revisions, and product dependencies. Derive user-facing
statuses such as Edits pending, Processing required, and Grid outdated. Provide
a recommended next action while allowing deliberate alternative workflows.

Source: [ProjectService.h, line and product state](../src/mbstudio/kernel/include/mbstudio/api/ProjectService.h#L25).

## 4. Recommended Desktop Design

Use Photoshop's document canvas, tool options, and configurable panels together
with MATLAB's project/data organization, command records, and computational
workflow.

These references describe real interaction structures:

- Adobe's [Photoshop workspace overview](https://helpx.adobe.com/photoshop/desktop/get-started/learn-the-basics/workspace-overview.html)
  identifies document windows, tools, panels, and an options bar for the selected
  tool.
- MathWorks' [MATLAB desktop layout overview](https://www.mathworks.com/help/matlab/matlab_env/overview-of-new-desktop-layout.html)
  describes configurable side panels and document/figure arrangements.

### 4.1 Workbench Regions

| Region | Responsibility |
| --- | --- |
| Custom title bar | Application/project identity, workspace access, and custom minimize, maximize/restore, and close controls; visually integrated with the workbench |
| Menu bar | File, Edit, View, Data, Process, Analysis, Window, Help |
| Tool options bar | Settings for the active selection, brush, measurement, or section tool |
| Left docks | Project data tree, survey lines, and layers; search, filters, and batch selection |
| Central document area | Map, terrain scene, sounding editor, and profiles; multiple documents and split views |
| Right docks | Selected-object properties, display settings, and processing parameters in coordinated panels |
| Bottom docks | Jobs, logs, command records, and processing history; collapsible when unused |
| Status bar | Active tool, selection, coordinates and units, data version, and task state |

### 4.2 Visual Direction

- Provide neutral dark-gray and light-gray themes.
- Use compact rows, fine separators, and small corner radii.
- Use the accent color for selection, focus, and the active tool.
- Reserve strong status colors for meaningful warnings and failures.
- Keep the central data view spacious by consolidating duplicate controls.
- Make active tools, unavailable actions, keyboard focus, and selection visible.
- Keep ordinary processing choices understandable through domain terminology;
  retain exact commands for reproducibility and advanced use.

### 4.3 Workspace Presets

Provide Overview, Bathymetry Editing, Modeling, Cartography, and Delivery presets.
A preset changes panel visibility and arrangement while preserving open documents
and their data. Allow users to save and reset layouts.

Programs, Formats, and Plugins should remain accessible through tool-management
entry points. Specialized data documents should receive the central workspace
attention during normal survey processing.

### 4.4 Custom Title Bar and Window Behavior

The custom title bar is a required part of the desktop design. Render it using
application-owned controls and styling, with the native system title bar hidden.
Do not display both native and custom title bars. Apply matching custom window
chrome to MB-Studio-owned top-level windows, including detached document windows,
floating panels, and application dialogs.

Requirements:

- Show application/project identity and a clear indication of unsaved changes
  when applicable. Keep long project names from displacing the window controls.
- Provide custom minimize, maximize/restore, and close buttons with consistent
  icons, hover/pressed states, accessible names, and keyboard focus behavior.
- Support dragging from unoccupied title-bar space and double-clicking that
  space to maximize or restore. Interactive controls must not initiate dragging.
- Preserve edge/corner resizing, minimum window size, appropriate resize cursors,
  and usable behavior when restoring or dragging a maximized window.
- Integrate with supported platform window-management behavior, including the
  Windows system menu, taskbar actions, keyboard shortcuts, and Snap behavior.
- Handle multiple monitors, display scaling changes, active/inactive window
  appearance, and light/dark themes without clipping or misplaced hit regions.
- Save and restore window geometry, keeping restored windows accessible when
  monitor arrangements change.

Use a reusable title-bar component and a window-management boundary for any
platform-specific behavior. Native window-manager APIs may handle movement,
resizing, and placement while the visible title bar remains application-drawn.
The specific integration must be selected and verified during implementation;
these requirements do not claim that the current application already supports
custom window chrome or the listed platform interactions.

## 5. MB-System Integration by User Workflow

The following table defines target workflows. It does not claim that every
interaction or underlying adapter is currently implemented.

| Module | User outcome | Required integration |
| --- | --- | --- |
| Data overview | Import data, identify formats, inspect tracks and coverage, locate problematic lines | MBIO metadata/navigation, datalist management, inspection services, explicit source versions |
| Bathymetry editing | Inspect multiple lines and pings, select soundings, flag or restore points, undo changes | Per-sounding access, existing edit replay, automatic cleaning, ESF persistence, reprocessing |
| Navigation and corrections | Inspect navigation/attitude curves, examine crossing lines, manage sound velocity and tides, compare corrections | Navigation adjustment services, profile/correction data, typed parameters, processing and comparison |
| Terrain and mosaics | Choose extent and resolution, generate products, inspect quality, compare versions in 2D/3D | Existing grid/mosaic recipes, coordinate handling, quality layers, product dependencies |
| Cartography | Compose bathymetric maps, contours, quality figures, and perspective illustrations with consistent map furniture | GMT plotting, project data references, figure templates, background generation, and preview |
| Delivery | Export products with clear coordinates and units, inspect provenance, reproduce processing | Export adapters, recorded input versions, parameters, tool versions, outputs, and logs |

Continue using the existing typed recipes for preprocessing, cleaning,
corrections, processing, gridding, mosaics, and export. Connect each recipe to
the selected data, its revision, and the products it invalidates or creates.

### 5.1 GMT's Role and Existing Foundation

Use GMT as the scientific cartography and figure-output backend. Qt owns the
desktop interface and interactive document behavior; a dedicated scene renderer
owns continuous 3D navigation, sounding picking, selection, and editing.

| Plotting requirement | GMT capability |
| --- | --- |
| Bathymetry, terrain, and quality-grid maps | `grdimage`: grid imaging, map projection, color palettes, and illumination |
| Depth contours and other isolines | `grdcontour`: contour generation, plotting, and labeling |
| Terrain perspective illustrations | `grdview`: perspective surface or mesh figures, with optional draping and illumination |
| Composed report figures | Map frames, legends, color scales, scale bars, annotations, and supported figure export paths |

Official references: [grdimage](https://docs.generic-mapping-tools.org/latest/grdimage.html),
[grdcontour](https://docs.generic-mapping-tools.org/latest/grdcontour.html),
[grdview](https://docs.generic-mapping-tools.org/latest/grdview).

The repository already registers `gmt` in its tool catalog and uses
`grdconvert` and `grd2xyz` in grid export recipes. This establishes a command
execution foundation for the proposed plotting integration. The active Grid
view currently uses the application's own image renderer; a dedicated GMT
cartography workspace has not been implemented or runtime-verified in this
review.

Sources: [ToolService.cpp, GMT catalog entry](../src/mbstudio/plugins/mbsystem/ToolService.cpp#L92),
[ProcessingService.cpp, grid export recipes](../src/mbstudio/plugins/mbsystem/ProcessingService.cpp#L477),
[GridRenderer.cpp, current image renderer](../src/mbstudio/plugins/mbsystem/GridRenderer.cpp).

GMT perspective output and interactive 3D viewing have separate responsibilities.
Changing GMT view parameters can generate a new perspective figure. Continuous
mouse-driven orbiting, point picking, brush selection, cross-section manipulation,
and undoable sounding edits belong to the interactive scene and editing services.
A GMT figure preview must not be presented as completion of the 3D editor.

### 5.2 Cartography Workspace and Plotting Jobs

Provide a Cartography workspace integrated with the project and document model:

1. Select project grids, quality products, and survey-line data, including their
   intended versions.
2. Choose a figure template and configure projection, extent, color palette,
   contours, illumination, legends, scale bars, labels, page size, and resolution.
3. Generate the figure as a background job with cancellation, logs, and clear
   failure feedback. Use a separate working directory and GMT session identity
   for each plotting job; prevent concurrent jobs from sharing mutable session
   state or output paths.
4. Preview the generated figure in a document, revise settings, and regenerate.
   Keep data inspection and selection connected to the source datasets rather
   than inferring values or point identities from preview pixels.
5. Save the figure configuration, generated command sequence, input versions,
   GMT version, outputs, and job log as project provenance.
6. Export the requested deliverables and allow the saved configuration to
   regenerate the figure later.

Provide reusable templates for bathymetry with contours, survey overview, grid
quality, and terrain perspective figures. Use compatible color ranges and units
across interactive views and exported figures, and identify display resampling
or interpolation when it affects interpretation.

### 5.3 Output and Deployment Requirements

Define and verify PNG/PDF figure export independently from numeric grid export.
For conversion paths using `psconvert`, deploy or locate Ghostscript and report
missing dependencies before starting the plotting job.
[GMT's psconvert documentation](https://docs.generic-mapping-tools.org/latest/psconvert.html)
describes its Ghostscript-based conversion. `grdimage -A` also offers direct
raster output, with different composition constraints described in the
[grdimage documentation](https://docs.generic-mapping-tools.org/latest/grdimage.html).

Check the exact bundled GMT version and its required modules, fonts, conversion
tools, and data paths in the deployment environment. Keep preview resolution,
export resolution, and georeferenced data export explicit. Save a complete figure
recipe so that successful command execution can be traced to a specific result.

## 6. 3D and Editing Requirements

### 6.1 First Delivery: Terrain Result Viewing

The initial 3D viewer should provide:

- A real grid-derived surface with explicit coordinate and vertical conventions.
- Orbit rotation, pan, zoom, fit-all, and fit-selection.
- Top, front, and side views; orthographic and perspective projection.
- Depth coloring, lighting, vertical exaggeration, and a readable legend.
- Position/depth picking, measurement, and profile inspection.
- Clear handling of NoData regions and interpolated cells.
- Saved camera and display state associated with the document.

Define consistent mouse and keyboard behavior before implementation. Test those
interactions at representative window sizes and display scaling settings.

### 6.2 Second Delivery: Sounding Inspection and Editing

Add:

- Point clouds containing soundings from multiple selected survey lines.
- Coloring by depth, intensity where available, survey line, and beam status.
- Point, rectangle, lasso, and brush selection with explicit selection modes.
- Flag/unflag operations, visible edited state, and undo/redo.
- Linked map, 3D, profile, and ping views.
- Saving edits, applying them through processing, and comparing the new result.

Each point must remain traceable to its source line, ping timestamp, sonar
multiplicity where applicable, and beam index. Rendering simplification must
preserve the relationship between a displayed point and the underlying sounding.

Editing must load existing edits and persist compatible edit events, then set
the processing parameters needed for `mbprocess` to apply them. MB-System's
[bathymetry editor integration guide](https://www3.mbari.org/products/mbsystem/batheditintegration/index.html)
documents the ESF event identity and the connection between edits, parameter
files, and processing.

### 6.3 Architecture Boundaries

Introduce focused boundaries for document state, sounding access, selection,
editing, and scene rendering. Names for any new services should be chosen during
implementation; these are responsibilities, not existing API claims.

- Documents own their view models and view state.
- Sounding access supplies identifiable data in suitable batches.
- Editing owns edit replay, changes, undo/redo, and persistence.
- Rendering consumes data without becoming the owner of processing truth.
- Task coordination controls conflicting reads/writes and result registration.
- Project state records revisions and product dependencies.

Keep the current kernel and service separation. Evaluate renderer candidates
against representative survey data, picking requirements, memory use, and the
supported deployment configuration before committing to an implementation.

## 7. Implementation Sequence and Acceptance Criteria

### Stage 1: Result Correctness and Traceability

Address conflicting jobs, output reservation, retry history, inspected data
versions, and projected-grid feedback. Establish revision/dependency state.

Acceptance criteria:

- Conflicting tasks cannot silently write the same output.
- Every project retry has a traceable attempt and an explicit output policy.
- Inspecting a processed line opens the identified processed version.
- Projected products open in an appropriate viewer, with accurate feedback.
- Changing edits or corrections marks affected processed data and products as
  requiring an update.

### Stage 2: Professional Workbench and Document Model

Introduce the custom title bar and window chrome, independent documents, split
views, coordinated properties panels, configurable docking, compact styling, and
workspace presets.

Acceptance criteria:

- The main window and other MB-Studio-owned top-level windows use title bars
  drawn by the application, with no visible native system title bar.
- Custom window controls, dragging, maximize/restore, resizing, supported window
  shortcuts, and platform window-management interactions are verified.
- Window chrome remains usable across the tested themes, display scaling
  settings, and monitor arrangements.
- Two different grids can remain open and be compared side by side.
- Selection and active-tool changes update the relevant controls consistently.
- Properties and display controls do not consume duplicate sidebars.
- Reopening a project restores document data and viewing state.
- The primary data view remains usable at the application's minimum supported
  window size and at the display scaling settings selected for verification.

### Stage 3: Terrain 3D Viewer

Deliver the terrain viewing interactions in Section 6.1.

Acceptance criteria:

- Rotation, pan, zoom, standard views, and projection switching work on a real
  generated grid.
- Picking reports values in the documented coordinates and units.
- NoData and interpolation status remain understandable in 3D.
- Camera and display settings survive project reopening.

### Stage 4: Sounding Editing Loop

Deliver sounding access, linked views, selection, flagging, undo/redo, edit
persistence, and reprocessing.

Acceptance criteria:

- Selected points map to the correct source soundings.
- Saved edits survive reopening and are applied during processing.
- The user can compare the original and edited/processed results.
- A new grid can be generated from the intended processed version with recorded
  provenance.

### Stage 5: Navigation and Correction Workspaces

Expand navigation adjustment, sound velocity, attitude, tide, and related
quality-control interactions around the established document and revision model.

Acceptance criteria should be defined for each calculation and its visual
comparison workflow. Exposing another executable is insufficient evidence that
the associated interactive workflow is complete.

### Stage 6: GMT Cartography and Delivery

Deliver the Cartography workspace, figure templates, isolated plotting jobs,
preview, saved configurations, and reproducible figure exports in Section 5.
This work can begin after Stage 2 and be delivered alongside terrain viewing;
completion of navigation or sounding editing is not a prerequisite.

Acceptance criteria:

- A real project grid produces a bathymetric figure with the selected projection,
  extent, color scale, contours, annotations, and scale bar.
- A GMT terrain perspective figure can be generated and previewed while the
  interactive 3D viewer retains its own navigation and selection behavior.
- Requested PNG/PDF output has the configured size, resolution, units, and labels.
- Missing plotting dependencies are identified before execution; failures and
  cancellation are visible in the task system.
- Concurrent plotting jobs use isolated directories, sessions, and outputs.
- Reopening a project restores figure settings, and a saved recipe can regenerate
  the result from its recorded input version without manual command editing.

## 8. Verification Plan

The existing GUI smoke test loads the workbench, visits pages and panels, checks
for QML warnings, and captures screenshots. It is useful for detecting loading
failures, but it does not establish correct mouse-driven workflows.

Source: [QmlUiTest.cpp, workbench smoke test](../src/mbstudio/tests/qmlui/QmlUiTest.cpp#L533).

Add focused regression coverage for the correctness findings and interaction
tests for the new capabilities:

- Duplicate product names while jobs are queued or running.
- Conflicting operations on the same survey line.
- Failed/cancelled job retry and project-history persistence.
- Original, preprocessed, and processed inspection paths.
- Projected product display and completion feedback.
- Independent documents and restored data/view state.
- Custom title-bar controls, keyboard access, drag/resize hit regions,
  maximize/restore, and supported platform window-management behavior.
- Custom chrome across themes, display scaling changes, monitor transitions, and
  saved-window restoration.
- Mouse selection, camera controls, flagging, undo/redo, and edit persistence.
- Downstream invalidation after input or correction changes.
- GMT figure generation, required-dependency checks, projection/extent settings,
  preview/export consistency, isolated sessions, and saved-recipe regeneration.

Use a real survey for end-to-end acceptance:

**Import -> inspect -> edit soundings -> save edits -> process -> generate a grid
-> compare 2D/3D results -> compose and export a GMT figure -> export data products
-> reopen the project and regenerate the saved figure.**

Choose a reference survey, expected results, target machines, and measurable
performance budgets before making performance or compatibility claims.

## 9. Relationship to Existing Planning Documents

Read this review alongside the existing
[UI Architecture Redesign and Feature Expansion Plan](architecture-redesign.md).
That document records earlier architectural goals and an implementation-status
table. This review adds source-based findings about the current behavior and
defines acceptance criteria for the next stage.

Update implementation status only when the relevant data flow and interaction
have been verified. Distinguish a command adapter, a parameter panel, an
interactive workspace, and an end-to-end validated workflow when reporting
feature completion.

## 10. Implementation Status (MB-Studio 0.3.0)

This section records what MB-Studio 0.3.0 implements for each finding and
stage, and how it is verified. Following Section 9, it distinguishes a
command adapter, a parameter panel, an interactive workspace and an
end-to-end validated workflow. "Automated" means a test of the CI suite
(Linux and Windows) exercises it with real MB-System programs and data;
"manual" means it still needs to be checked by hand on Windows.

### Stage 1: Result Correctness and Traceability

| Finding | Implementation | Verification |
| --- | --- | --- |
| F1 conflicting jobs | Jobs declare read and write resources (`line:<id>`, `file:<path>`); the job service never runs conflicting jobs together and keeps their submission order; the Jobs panel names the job a queued job waits for. Output names are reserved while their job is queued or running. | Automated: `JobService.SerialisesConflictingJobs`; `ProjectTest.ReservesOutputsSerialisesAndRetries` (clean before grid, two grids of the same name get different files). |
| F2 retry history | Project jobs are not retryable by the job service; the project repeats them from the recorded request as a new attempt (history entry with attempt number and the attempt it repeats, new job folder, new outputs). Requests are stored in the project file, so retries work in later sessions; a request can be loaded into its panel to change it. | Automated: project test above, host test (`project.retry`), UI test (retry from the Jobs model), Windows installer smoke test. |
| F3 inspected version | Line documents resolve the original, preprocessed and processed files through the project service and show which one is open; the version can be switched in the properties panel. | Automated: versions after import and after `mbprocess` (`ProjectTest.EditsProcessingAndOutdatedProducts`), UI smoke test. Before/after comparison: two line documents side by side (split view). |
| F4 projected grids | Grids record their projection (`Projection:` remark of `mbgrid`/`mbmosaic`); UTM and local TM grids are warped onto the geographic map and their values read at the cursor; the completion message reports whether the layer is on the map or why not. | Automated: `mbgrid -JU` grid read as `UTM10N` and placed at the right position; transverse Mercator checked against reference values. Manual: a projected EM122 grid on the map. |
| F8 states | Lines carry edit, correction and source revisions and the revisions of their processed file; products record the data stamp of every input line. States: unprocessed, processing required, edits/corrections pending, processed; products current, pending, outdated; a recommended next step; *Make again* regenerates a product from its recipe. | Automated: the full editing loop (edits → pending → process → outdated → regenerate → current) in `ProjectTest.EditsProcessingAndOutdatedProducts`. |

### Stage 2: Workbench and Document Model

| Requirement | Implementation | Verification |
| --- | --- | --- |
| Custom title bar | Title bar drawn by MB-Studio (identity, menus, unsaved-edits mark, accessible window buttons). On Windows the native frame styles are kept and `WM_NCCALCSIZE`/`WM_NCHITTEST` are answered: caption drag, double click, Snap, system menu, edge and corner resizing, maximised windows kept on the monitor work area. Elsewhere `startSystemMove`/`startSystemResize`. Geometry is restored onto existing screens. `MBSTUDIO_NATIVE_TITLEBAR=1` falls back to the system title bar. | Automated: the hit test, the control detection on the real title bar and the screen fitting. **Manual (open): Snap, multiple monitors, scaling changes and the system menu on Windows 10/11.** Detached document windows and floating panels do not exist yet, so only the main window needs chrome; file dialogs are the system's. |
| Independent documents, split views | One document object per grid, 3D, line, editor and figure tab; two groups side by side; tabs, groups and each document's view state are saved in the project and restored. | Automated: workspace model test (documents, groups, view state round trip), UI smoke test (two grids and a grid beside its 3D view). |
| No duplicate sidebars | The grid view's sidebar moved into the context-sensitive properties panel (grid, 3D, editor, line and figure pages); tool options bar for the active document; status bar with tool, selection, cursor and data version. | UI smoke test and screenshots. |
| Presets, menus, styling | Menus File, Edit, View, Data, Process, Analysis, Window, Help; presets Overview, Bathymetry editing, Modelling, Cartography, Delivery with a saved custom layout; neutral grey themes, compact controls. | UI smoke test. Panels are resizable docks in fixed regions; free docking and floating panels are not implemented. |

### Stage 3: Terrain 3D Viewer

Implemented as an interactive workspace with a software renderer (works with
every scene graph backend, including the tests' software backend; depth and
identifier buffers for exact picking): orbit, pan, zoom, fit, top/front/side
views, perspective/orthographic, colour tables, lighting, vertical
exaggeration, legend, picking of position and value from the grid,
measurement, profiles, NoData holes, stippled interpolated cells, camera and
display saved with the project. Automated: renderer, picking, measurement and
view state tests. Manual: performance with large grids (the surface is drawn
from a level of detail of at most 640 × 640 nodes; 180 × 180 while moving).

### Stage 4: Sounding Editing Loop

Implemented: soundings of selected lines with saved edits applied (MBIO,
`mb_esf_apply`), 3D point cloud with rectangle, lasso, brush and point
selection, flag/unflag, undo/redo, colouring by depth, line, flag state or
beam, a linked ping view, the identity of every sounding (line, ping time,
multiplicity, beam), saving through a project job to the edit save file and
the `mbprocess` parameter file, and the processing loop. Automated: edit
events written and re-applied (`SoundingService.AppliesAndSavesEditEvents`),
edits applied by `mbprocess` to the processed file, the editor saving through
the project (UI test). Not yet: intensity colouring, a map-linked selection,
editing more than about six million soundings at once.

### Stage 5: Navigation and Correction Workspaces

Not started in 0.3.0 (corrections remain a parameter panel).

### Stage 6: GMT Cartography and Delivery

Implemented: a figure panel (bathymetric map with contours, survey overview,
grid quality map, perspective view; colour table, illumination, contours,
colour scale, scale bar, title, PNG/PDF/PostScript, size and resolution),
jobs in private GMT sessions and temporary folders, Ghostscript located and
reported before the job, figures as products with their recipe (figure tab,
*Make again*, *Change settings*). Automated: recipe tests; PostScript figures
made by GMT in two parallel jobs on Linux (where GMT is installed) and in the
installed Windows application. Manual: PNG/PDF output on Windows with
Ghostscript installed, the quality of the composed maps for EM122 data.

### End-to-end acceptance

The real-survey acceptance of Section 8 (PatchSurveyEM122, 355 MB) cannot run
in CI and is still open: import → inspect → edit soundings → save → process →
grid → 2D/3D comparison → GMT figure → export → reopen and regenerate.
