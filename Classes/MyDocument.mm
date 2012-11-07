//
//  MyDocument.m
//  OpenGLEditor
//
//  Created by Filip Kunc on 6/29/09.
//  For license see LICENSE.TXT
//

#import "MyDocument.h"
#import "ItemManipulationState.h"
#import "IndexedItem.h"
#import <sstream>

@implementation MyDocument

@synthesize items;

- (id)init
{
    self = [super init];
    if (self) 
	{
        // Add your subclass-specific initialization here.
        // If an error occurs here, send a [self release] message and return nil.
		items = [[ItemCollection alloc] init];
		itemsController = [[OpenGLManipulatingController alloc] init];
		meshController = [[OpenGLManipulatingController alloc] init];
		
		[itemsController setModel:items];
		manipulated = itemsController;

		[itemsController addSelectionObserver:self];
		[meshController addSelectionObserver:self];
		
		manipulationFinished = YES;
		oldManipulations = nil;
		oldMeshState = nil;
		
		views = [[NSMutableArray alloc] init];
		oneView = nil;
        
        currentManipulator = ManipulatorTypeDefault;
        
        NSUndoManager *undo = [self undoManager];
        [undo setLevelsOfUndo:100];
    }
    return self;
}
						   
- (void)dealloc
{
	[itemsController removeSelectionObserver:self];
	[meshController removeSelectionObserver:self];
}

- (void)awakeFromNib
{
    //[NSColor setIgnoresAlpha:NO];
    
	[editModePopUp selectItemWithTag:0];
    [scriptWindowController setDelegate:self];
    [[scriptPullDown menu] setDelegate:self];
    [self menuNeedsUpdate:[scriptPullDown menu]];
    
	[views addObject:viewTop];
	[views addObject:viewLeft];
	[views addObject:viewFront];
	[views addObject:viewPerspective];
	
	for (OpenGLSceneView *view in views)
	{ 
		[view setManipulated:manipulated]; 
		[view setDisplayed:itemsController];
		[view setDelegate:self];
	};
	
	[viewTop setCameraMode:CameraModeTop];
	[viewLeft setCameraMode:CameraModeLeft];
	[viewFront setCameraMode:CameraModeFront];
	[viewPerspective setCameraMode:CameraModePerspective];
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
    NSMenuItem *firstItem = [menu itemAtIndex:0];
    [menu removeAllItems];
    [menu addItem:firstItem];
    
    NSArray *scripts = [scriptWindowController scripts];
    NSUInteger index = 1;
    for (NSString *script in scripts)
    {
        NSMenuItem *item = [menu addItemWithTitle:script action:@selector(runScriptAction:) keyEquivalent:@""];
        item.keyEquivalentModifierMask = NSCommandKeyMask;
        item.keyEquivalent = [NSString stringWithFormat:@"%lu", index];
        
        if (index != 0 && ++index > 9)
            index = 0;
    }
}

- (IBAction)runScriptAction:(id)sender
{
    NSString *scriptName = [[scriptPullDown selectedItem] title];
    [scriptWindowController runScriptWithName:scriptName];
}

- (enum ManipulatorType)currentManipulator
{
    return currentManipulator;
}

- (void)setCurrentManipulator:(enum ManipulatorType)value;
{
    currentManipulator = value;
    [itemsController setCurrentManipulator:value];
    [meshController setCurrentManipulator:value];
}

- (NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
    return proposedOptions;    
}

- (void)setNeedsDisplayExceptView:(OpenGLSceneView *)view
{
	for (OpenGLSceneView *v in views)
	{ 
		if (v != view)
			[v setNeedsDisplay:YES]; 
	}
}

- (void)setNeedsDisplayOnAllViews
{
	for (OpenGLSceneView *v in views)
	{
		[v setNeedsDisplay:YES];
	}
}

- (id<OpenGLManipulating>)manipulated
{
	return manipulated;
}

- (void)setManipulated:(id<OpenGLManipulating>)value
{
	manipulated = value;
	
	for (OpenGLSceneView *view in views)
	{ 
		[view setManipulated:value];
		[view setNeedsDisplay:YES];
	};

	if (manipulated == itemsController)
	{
		[editModePopUp selectItemWithTag:EditModeItems];
	}
	else if (manipulated == meshController)
	{
		int meshTag = [self currentMesh]->selectionMode() + 1;
		[editModePopUp selectItemWithTag:meshTag];
	}
}

- (void)updateManipulatedSelection
{
    [manipulated updateSelection];
}

- (Mesh2 *)currentMesh
{
	if (manipulated == meshController)
		return [(Item *)[meshController model] mesh];
    if (manipulated == itemsController)
        return [(ItemCollection *)[itemsController model] currentMesh];
	return nil;
}

- (MyDocument *)prepareUndoWithName:(NSString *)actionName
{
	NSUndoManager *undo = [self undoManager];
	MyDocument *document = [undo prepareWithInvocationTarget:self];
	if (![undo isUndoing])
		[undo setActionName:actionName];
	return document;
}

- (void)addItemWithType:(enum MeshType)type steps:(uint)steps;
{
	Item *item = [[Item alloc] init];
	Mesh2 *mesh = [item mesh];
    mesh->make(type, steps);
	
	NSString *name = Mesh2::descriptionOfMeshType(type);
	
	MyDocument *document = [self prepareUndoWithName:[NSString stringWithFormat:@"Add %@", name]];
	[document removeItemWithType:type steps:steps];
	
	[items addItem:item];
    [textureBrowserWindowController setItems:items];	
	[itemsController changeSelection:NO];
	[items setSelected:YES atIndex:[items count] - 1];
	[itemsController updateSelection];
	[self setManipulated:itemsController];
}

- (void)removeItemWithType:(enum MeshType)type steps:(uint)steps
{
	NSString *name = Mesh2::descriptionOfMeshType(type);
	
	MyDocument *document = [self prepareUndoWithName:[NSString stringWithFormat:@"Remove %@", name]];
	[document addItemWithType:type steps:steps];
		
	[items removeLastItem];
    [textureBrowserWindowController setItems:items];
	[itemsController changeSelection:NO];
	[self setManipulated:itemsController];
}

- (float)selectionX
{
	return [manipulated selectionX];
}

- (float)selectionY
{
	return [manipulated selectionY];
}

- (float)selectionZ
{
	return [manipulated selectionZ];
}

- (void)setSelectionX:(float)value
{
	[self manipulationStartedInView:nil];
	[manipulated setSelectionX:value];
	[self manipulationEndedInView:nil];
}

- (void)setSelectionY:(float)value
{
	[self manipulationStartedInView:nil];
	[manipulated setSelectionY:value];
	[self manipulationEndedInView:nil];
}

- (void)setSelectionZ:(float)value
{
	[self manipulationStartedInView:nil];
	[manipulated setSelectionZ:value];
	[self manipulationEndedInView:nil];
}

- (BOOL)selectionColorEnabled
{
    return manipulated.selectionColorEnabled;
}

- (NSColor *)selectionColor
{
    return manipulated.selectionColor;
}

- (void)setSelectionColor:(NSColor *)selectionColor
{
    [self meshOnlyActionWithName:@"Change Color" block:^ { manipulated.selectionColor = selectionColor; }];
}

- (void)setNilValueForKey:(NSString *)key
{
	[self setValue:[NSNumber numberWithFloat:0.0f] forKey:key];
}

- (void)observeValueForKeyPath:(NSString *)keyPath 
					  ofObject:(id)object 
						change:(NSDictionary *)change
					   context:(void *)context
{
    if (manipulationFinished)
    {
        [self willChangeValueForKey:keyPath];
        [self didChangeValueForKey:keyPath];
    }
}

- (void)swapManipulationsWithOld:(NSMutableArray *)old current:(NSMutableArray *)current
{
	NSAssert([old count] == [current count], @"old count == current count");
	[items setCurrentManipulations:old];
	
	MyDocument *document = [self prepareUndoWithName:@"Manipulations"];	
	[document swapManipulationsWithOld:current current:old];
	
	[itemsController updateSelection];
	[self setManipulated:itemsController];
}

- (void)swapAllItemsWithOld:(NSMutableArray *)old
					current:(NSMutableArray *)current
				 actionName:(NSString *)actionName
{
	[items setAllItems:old];

	MyDocument *document = [self prepareUndoWithName:actionName];
	[document swapAllItemsWithOld:current
						  current:old
					   actionName:actionName];
	
	[itemsController updateSelection];
	[self setManipulated:itemsController];
}

- (void)swapMeshStateWithOld:(MeshState *)old
					 current:(MeshState *)current 
				  actionName:(NSString *)actionName
{
	[items setCurrentMeshState:old];
	Item *item = [items itemAtIndex:[old itemIndex]];
	
    [meshController setModel:item];
    [meshController setPosition:[item position] 
                       rotation:[item rotation] 
                          scale:[item scale]];
    
	MyDocument *document = [self prepareUndoWithName:actionName];
	[document swapMeshStateWithOld:current
						   current:old
						actionName:actionName];
	
	[itemsController updateSelection];
	[meshController updateSelection];
    
	[self setManipulated:meshController];
}

- (void)allItemsActionWithName:(NSString *)actionName block:(void (^)())action
{
	MyDocument *document = [self prepareUndoWithName:actionName];
	NSMutableArray *oldItems = [items allItems];

	action();
    
    [textureBrowserWindowController setItems:items];
	
	NSMutableArray *currentItems = [items allItems];
	[document swapAllItemsWithOld:oldItems 
						  current:currentItems
					   actionName:actionName];	
}

- (void)meshActionWithName:(NSString *)actionName block:(void (^)())action
{
	MyDocument *document = [self prepareUndoWithName:actionName];
	MeshState *oldState = [items currentMeshState];
	
	action();
	
	MeshState *currentState = [items currentMeshState];
	[document swapMeshStateWithOld:oldState 
						   current:currentState
						actionName:actionName];
}

- (void)manipulationStartedInView:(OpenGLSceneView *)view
{
	manipulationFinished = NO;
	
	if (manipulated == itemsController)
	{
		oldManipulations = [items currentManipulations];
	}
	else if (manipulated == meshController)
	{
		oldMeshState = [items currentMeshState];
	}
}

- (void)manipulationEndedInView:(OpenGLSceneView *)view
{
	manipulationFinished = YES;
	
	if (manipulated == itemsController)
	{
		MyDocument *document = [self prepareUndoWithName:@"Manipulations"];
		[document swapManipulationsWithOld:oldManipulations current:[items currentManipulations]];
		oldManipulations = nil;
        
        [itemsController willChangeSelection];
        [itemsController didChangeSelection];
	}
	else if (manipulated == meshController)
	{
		MyDocument *document = [self prepareUndoWithName:@"Mesh Manipulation"];
		[document swapMeshStateWithOld:oldMeshState current:[items currentMeshState] actionName:@"Mesh Manipulation"];
		oldMeshState = nil;
        
        [meshController willChangeSelection];
        [meshController didChangeSelection];
	}
	
	[self setNeedsDisplayExceptView:view];
}

- (NSColor *)brushColor
{
    return [texturePaintToolWindowController brushColor];
}

- (float)brushSize
{
    return [texturePaintToolWindowController brushSize];
}

- (void)selectionChangedInView:(OpenGLSceneView *)view
{
	[self setNeedsDisplayExceptView:view];
}

- (IBAction)addPlane:(id)sender
{
    [self addItemWithType:MeshTypePlane steps:0];
}

- (IBAction)addCube:(id)sender
{
	[self addItemWithType:MeshTypeCube steps:0];
}

- (IBAction)addCylinder:(id)sender
{
	itemWithSteps = MeshTypeCylinder;
	[addItemWithStepsSheetController beginSheetWithProtocol:self];
}

- (IBAction)addSphere:(id)sender
{
	itemWithSteps = MeshTypeSphere;
	[addItemWithStepsSheetController beginSheetWithProtocol:self];
}

- (void)addIcosahedron:(id)sender
{
    [self addItemWithType:MeshTypeIcosahedron steps:0];
}

- (void)addItemWithSteps:(uint)steps
{
	[self addItemWithType:itemWithSteps steps:steps];
}

- (void)editMeshWithMode:(enum MeshSelectionMode)mode
{
	NSInteger index = [itemsController lastSelectedIndex];
	if (index > -1)
	{
		Item *item = [items itemAtIndex:index];
        [item mesh]->setSelectionMode(mode);
		[meshController setModel:item];
		[meshController setPosition:[item position] 
						   rotation:[item rotation] 
							  scale:[item scale]];
		[self setManipulated:meshController];
	}
}

- (void)editItems
{
    Mesh2 *currentMesh = [self currentMesh];
    if (currentMesh)
        currentMesh->setSelectionMode(MeshSelectionModeVertices);
	[itemsController setModel:items];
	[itemsController setPosition:Vector3D()
						rotation:Quaternion()
						   scale:Vector3D(1, 1, 1)];
	[self setManipulated:itemsController];
}

- (IBAction)changeEditMode:(id)sender
{
	EditMode mode = (EditMode)[[editModePopUp selectedItem] tag];
    Mesh2 *currentMesh = [self currentMesh];
    
    if (!currentMesh)
        [editModePopUp selectItemWithTag:EditModeItems];
    
	switch (mode)
	{
		case EditModeItems:
			[self editItems];
			break;
		case EditModeVertices:
			[self editMeshWithMode:MeshSelectionModeVertices];
			break;
		case EditModeTriangles:
			[self editMeshWithMode:MeshSelectionModeTriangles];
			break;
		case EditModeEdges:
			[self editMeshWithMode:MeshSelectionModeEdges];
			break;
	}
}

- (enum ViewMode)viewMode
{
    return manipulated.viewMode;
}

- (void)setViewMode:(enum ViewMode)viewMode
{
    manipulated.viewMode = viewMode;
    [self setNeedsDisplayOnAllViews];
}

- (IBAction)mergeSelected:(id)sender
{
	if ([manipulated selectedCount] <= 0)
		return;
	
	if (manipulated == itemsController)
	{
		[self allItemsActionWithName:@"Merge" block:^ { [items mergeSelectedItems]; }];
	}
	else if (manipulated == meshController)
	{
		[self meshActionWithName:@"Merge" block:^ { [self currentMesh]->mergeSelected(); }]; 
	}
	
	[manipulated updateSelection];
	[self setNeedsDisplayOnAllViews];
}

- (void)meshOnlyActionWithName:(NSString *)actionName block:(void (^)())action
{    
    if ([self currentMesh] == nil)
        return;
	
	BOOL startManipulation = NO;
	if (!manipulationFinished)
	{
		startManipulation = YES;
		[self manipulationEndedInView:nil];
	}
	
    [self meshActionWithName:actionName block:action];
	
    [manipulated updateSelection];
	[self setNeedsDisplayOnAllViews];
	
	if (startManipulation)
	{
		[self manipulationStartedInView:nil];
	}
}

- (IBAction)splitSelected:(id)sender
{
    [self meshOnlyActionWithName:@"Split" block:^ { [self currentMesh]->splitSelected(); }];
}

- (IBAction)flipSelected:(id)sender
{
    [self meshOnlyActionWithName:@"Flip" block:^ { [self currentMesh]->flipSelected(); }];
}

- (IBAction)duplicateSelected:(id)sender
{	
	if ([manipulated selectedCount] <= 0)
		return;
	
	BOOL startManipulation = NO;
	if (!manipulationFinished)
	{
		startManipulation = YES;
		[self manipulationEndedInView:nil];
	}
	
	if (manipulated == itemsController)
	{
		NSMutableArray *selection = [items currentSelection];
		MyDocument *document = [self prepareUndoWithName:@"Duplicate"];
		[document undoDuplicateSelected:selection];
		[manipulated duplicateSelected];
	}
	else if (manipulated == meshController)
	{
		[self meshActionWithName:@"Duplicate" block:^ { [manipulated duplicateSelected]; }];
	}
	
    [manipulated updateSelection];
	[self setNeedsDisplayOnAllViews];
	
	if (startManipulation)
	{
		[self manipulationStartedInView:nil];
	}
}

- (void)redoDuplicateSelected:(NSMutableArray *)selection
{
	[self setManipulated:itemsController];
	[items setCurrentSelection:selection];
	[manipulated duplicateSelected];
	
	MyDocument *document = [self prepareUndoWithName:@"Duplicate"];
	[document undoDuplicateSelected:selection];
	
	[manipulated updateSelection];
	[self setNeedsDisplayOnAllViews];
}

- (void)undoDuplicateSelected:(NSMutableArray *)selection
{	
	[self setManipulated:itemsController];
	uint duplicatedCount = [selection count];
	[items removeItemsInRange:NSMakeRange([items count] - duplicatedCount, duplicatedCount)];
	[items setCurrentSelection:selection];

	MyDocument *document = [self prepareUndoWithName:@"Duplicate"];
	[document redoDuplicateSelected:selection];
		
	[manipulated updateSelection];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)deleteSelected:(id)sender
{
	if ([manipulated selectedCount] <= 0)
		return;
	
	if (manipulated == itemsController)
	{
		NSMutableArray *currentItems = [items currentItems];
		MyDocument *document = [self prepareUndoWithName:@"Delete"];
		[document undoDeleteSelected:currentItems];
		[manipulated removeSelected];
        [textureBrowserWindowController setItems:items];
	}
	else if (manipulated == meshController)
	{
		[self meshActionWithName:@"Delete" block:^ { [manipulated removeSelected]; }];
	}
	
	[self setNeedsDisplayOnAllViews];
}

- (void)redoDeleteSelected:(NSMutableArray *)selectedItems
{
	[self setManipulated:itemsController];
	[items setSelectionFromIndexedItems:selectedItems];
	[manipulated removeSelected];
    [textureBrowserWindowController setItems:items];
	
	MyDocument *document = [self prepareUndoWithName:@"Delete"];
	[document undoDeleteSelected:selectedItems];

	[itemsController updateSelection];
	[self setNeedsDisplayOnAllViews];
}

- (void)undoDeleteSelected:(NSMutableArray *)selectedItems
{
	[self setManipulated:itemsController];
	[items setCurrentItems:selectedItems];
    [textureBrowserWindowController setItems:items];
	
	MyDocument *document = [self prepareUndoWithName:@"Delete"];
	[document redoDeleteSelected:selectedItems];
	
	[itemsController updateSelection];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)subdivision:(id)sender
{
    [self meshOnlyActionWithName:@"Subdivision" block:^ { [self currentMesh]->openSubdivision(); }];
}

+ (BOOL)softSelection
{
    return Mesh2::useSoftSelection();
}

+ (void)setSoftSelection:(BOOL)value
{
    Mesh2::setUseSoftSelection(value);
}

- (IBAction)softSelection:(id)sender
{
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    [MyDocument setSoftSelection:![MyDocument softSelection]];
    [menuItem setState:[MyDocument softSelection]];
}

- (IBAction)changeManipulator:(id)sender
{
	self.currentManipulator = (ManipulatorType)[sender tag];
	for (OpenGLSceneView *view in views)
	{ 
		[view setCurrentManipulator:currentManipulator]; 
	}
}

- (IBAction)selectAll:(id)sender
{
	[[self manipulated] changeSelection:YES];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)invertSelection:(id)sender
{
	[[self manipulated] invertSelection];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)hideSelected:(id)sender
{
	[[self manipulated] hideSelected];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)unhideAll:(id)sender
{
	[[self manipulated] unhideAll];
	[self setNeedsDisplayOnAllViews];
}

- (IBAction)detachSelected:(id)sender
{
    [self meshOnlyActionWithName:@"Detach" block:^ { [self currentMesh]->detachSelected(); }];
}

- (IBAction)extrudeSelected:(id)sender
{
    [self meshOnlyActionWithName:@"Extrude" block:^ { [self currentMesh]->extrudeSelectedTriangles(); }];
}

- (IBAction)cleanTexture:(id)sender
{
    Mesh2 *mesh = [self currentMesh];
    if (mesh)
    {
        mesh->cleanTexture();
        [self setNeedsDisplayOnAllViews];
    }
}

- (IBAction)resetTexCoords:(id)sender
{
    [self meshOnlyActionWithName:@"Reset Texture Coordinates" block:^ 
    { 
        Mesh2 *mesh = [self currentMesh];
        mesh->resetTriangleCache();
        mesh->makeTexCoords();
        mesh->makeEdges();
    }];
}

- (IBAction)triangulate:(id)sender
{
    if (manipulated == meshController)
    {
        [self meshOnlyActionWithName:@"Triangulate" block:^ { [self currentMesh]->triangulateSelectedQuads(); }];
    }
    else if (manipulated == itemsController)
    {
        [self allItemsActionWithName:@"Triangulate" block:^
        {
            for (Item *item in items)
                item.mesh->triangulate();
        }];
    }
}

- (IBAction)viewTexturePaintTool:(id)sender
{
    [texturePaintToolWindowController showWindow:nil];
}

- (IBAction)viewTextureBrowser:(id)sender
{
    [textureBrowserWindowController setItems:items];
    [textureBrowserWindowController showWindow:nil];
}

- (void)viewScriptEditor:(id)sender
{
    [scriptWindowController showWindow:nil];
}

- (BOOL)texturePaintEnabled
{
    if (texturePaintToolWindowController.isWindowLoaded)
    {
        if (texturePaintToolWindowController.window.isVisible)
            return YES;
    }
    return NO;
}

- (NSString *)windowNibName
{
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"MyDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
}

#pragma mark Archivation

+ (BOOL)autosavesInPlace
{
    return YES;
}

- (void)windowWillEnterVersionBrowser:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *)[notification object];
    [window.toolbar setVisible:NO];
}

- (void)windowDidExitVersionBrowser:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *)[notification object];
    [window.toolbar setVisible:YES];
}

- (BOOL)readFromFileWrapper:(NSFileWrapper *)dirWrapper ofType:(NSString *)typeName error:(NSError *__autoreleasing *)outError
{
    if ([typeName isEqualToString:@"model3D"])
        return [self readFromModel3D:[dirWrapper regularFileContents]];

    if ([typeName isEqualToString:@"Wavefront Object"])
        return [self readFromWavefrontObject:[dirWrapper regularFileContents]];
    
    NSFileWrapper *modelWrapper = [[dirWrapper fileWrappers] objectForKey:@"Geometry.model3D"];
    NSData *modelData = [modelWrapper regularFileContents];
    [self readFromModel3D:modelData];
    
    [[dirWrapper fileWrappers] enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) 
    {
        NSString *textureName = (NSString *)key;
        if ([textureName hasSuffix:@".png"])
        {
            textureName = [textureName stringByDeletingPathExtension];
            
            NSFileWrapper *textureWrapper = (NSFileWrapper *)obj;
            NSData *textureData = [textureWrapper regularFileContents];
            
            NSImage *image = [[NSImage alloc] initWithData:textureData];
            uint index = [textureName substringFromIndex:@"Texture".length].integerValue;
            Item *item = [items itemAtIndex:index];
            FPTexture *texture = item.mesh->texture();
            [texture setCanvas:image];
        }        
    }];
    
    return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError *__autoreleasing *)outError
{
    if ([typeName isEqualToString:@"model3D"])
        return [[NSFileWrapper alloc] initRegularFileWithContents:[self dataOfModel3D]];
    
    if ([typeName isEqualToString:@"Wavefront Object"])
        return nil;
    
    NSFileWrapper *dirWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:nil];
    
    [dirWrapper addRegularFileWithContents:[self dataOfModel3D]
                         preferredFilename:@"Geometry.model3D"];
    
    int i = 0;
    
    for (Item *item in items)
    {
        NSImage *image = item.mesh->texture().canvas;
        NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
        
        NSData *imageData = [bitmap representationUsingType:NSPNGFileType properties:nil];

        
        [dirWrapper addRegularFileWithContents:imageData 
                             preferredFilename:[NSString stringWithFormat:@"Texture%.2i.png", i]];
        
        i++;
    }
    
    return dirWrapper;
}

- (BOOL)readFromModel3D:(NSData *)data
{
    MemoryReadStream *stream = [[MemoryReadStream alloc] initWithData:data];
    
    unsigned int version = 0;
    
    [stream readBytes:&version length:sizeof(unsigned int)];
    
    if (version < ModelVersionFirst || version > ModelVersionLatest)
        return NO;
    
    [stream setVersion:version];
    ItemCollection *newItems = [[ItemCollection alloc] initWithReadStream:stream];
    items = newItems;
    [itemsController setModel:items];
    [itemsController updateSelection];
    [self setManipulated:itemsController];

    return YES;
}

- (NSData *)dataOfModel3D
{
    NSMutableData *data = [[NSMutableData alloc] init];
    MemoryWriteStream *stream = [[MemoryWriteStream alloc] initWithData:data];
    
    unsigned int version = ModelVersionLatest;
    [stream setVersion:version];
    [stream writeBytes:&version length:sizeof(unsigned int)];
    [items encodeWithWriteStream:stream];
    return data;
}

- (BOOL)readFromWavefrontObject:(NSData *)data
{
    NSString *fileContents = [NSString stringWithUTF8String:(const char *)[data bytes]];
    string str = [fileContents UTF8String];
    stringstream ssfile;
    ssfile << str;
    
    vector<Vector3D> vertices;
    vector<Vector3D> texCoords;
    vector<TriQuad> triangles;
    vector<uint> groups;
    
    bool hasTexCoords = false;
    bool hasNormals = false;
    
    while (!ssfile.eof())
    {
        string line;
        getline(ssfile, line);
        stringstream ssline;
        ssline << line;
        
        string prefix;
        ssline >> prefix;
        
        if (prefix == "#")
        {
            // # This is a comment
            continue;
        }
        else if (prefix == "g")
        {
            // g group_name
            groups.push_back(triangles.size());
        }
        else if (prefix == "v")
        {
            // v -5.79346 -1.38018 42.63113
            Vector3D v;
            ssline >> v.x >> v.y >> v.z;
            
            swap(v.y, v.z);
            v.z = -v.z;
            
            vertices.push_back(v);
        }
        else if (prefix == "vt")
        {
            // vt 0.12528 -0.64560
            Vector3D vt;
            ssline >> vt.x >> vt.y >> vt.z;
            
            vt.z = 0.0f;
            
            texCoords.push_back(vt);
            hasTexCoords = true;
        }
        else if (prefix == "vn")
        {
            // vn -0.78298 -0.13881 -0.60637
            hasNormals = true;
        }
        else if (prefix == "f")
        {
            // f  v1 v2 v3 v4 ...
            // f  v1/vt1 v2/vt2 v3/vt3 ...
            // f  v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
            // f  v1//vn1 v2//vn2 v3//vn3 ...
            
            // f  187/1/1 204/2/2 185/3/3

            TriQuad triQuad;
            for (uint i = 0; i < 4; i++)
            {
                uint vi, ti, ni;
                char c;
                
                if (!hasTexCoords && !hasNormals)
                    ssline >> vi;
                else if (hasTexCoords && !hasNormals)
                    ssline >> vi >> c >> ti;
                else if (!hasTexCoords && hasNormals)
                    ssline >> vi >> c >> c >> ni;
                else if (hasTexCoords && hasNormals)
                    ssline >> vi >> c >> ti >> c >> ni;
                
                triQuad.vertexIndices[i] = vi - 1;
                triQuad.texCoordIndices[i] = ti - 1;
            }
            triQuad.isQuad = ssline.good();
            triangles.push_back(triQuad);
        }
    }
    
    Mesh2 *mesh = new Mesh2();
    if (!hasTexCoords)
        mesh->fromIndexRepresentation(vertices, vertices, triangles);
    else
        mesh->fromIndexRepresentation(vertices, texCoords, triangles);
    
    mesh->flipAllTriangles();
    
    mesh->setSelectionMode(MeshSelectionModeTriangles);
    
    ItemCollection *newItems = [[ItemCollection alloc] init];

    for (uint i = 0; i < groups.size(); i++)
    {
        for (uint j = 0; j < mesh->triangleCount(); j++)
            mesh->setSelectedAtIndex(false, j);
        
        for (uint j = groups.at(i), end = i + 1 < groups.size() ? groups.at(i + 1) : mesh->triangleCount(); j < end; j++)
            mesh->setSelectedAtIndex(true, j);
        
        Item *item = [[Item alloc] initFromSelectedTrianglesInMesh:mesh];
        [newItems addItem:item];
    }
    
    if (groups.empty())
    {
        Item *item = [[Item alloc] initWithMesh:mesh];
        [newItems addItem:item];
    }
    else
    {
        delete mesh;
    }
    
    items = newItems;
    [itemsController setModel:items];
    [itemsController updateSelection];
    [self setManipulated:itemsController];
    
    return YES;
}

- (NSData *)dataOfWavefrontObject
{
    return nil;
}

#pragma mark Splitter sync

- (CGFloat)splitView:(NSSplitView *)splitView 
constrainSplitPosition:(CGFloat)proposedPosition 
		 ofSubviewAt:(NSInteger)dividerIndex
{
	if (oneView)
		return 0.0f;
	
	return proposedPosition;
}

// fix for issue four-views works independently on Mac version
- (void)splitViewDidResizeSubviews:(NSNotification *)notification
{
	if (oneView)
		return;
	
	NSSplitView *splitView = (NSSplitView *)[notification object];
	if (splitView == mainSplit)
		return;
	
	NSView *topSubview0 = (NSView *)[[topSplit subviews] objectAtIndex:0];
	NSView *topSubview1 = (NSView *)[[topSplit subviews] objectAtIndex:1];
	
	NSView *bottomSubview0 = (NSView *)[[bottomSplit subviews] objectAtIndex:0];
	NSView *bottomSubview1 = (NSView *)[[bottomSplit subviews] objectAtIndex:1];
	
	// we are interested only in width change
	if (fabsf([bottomSubview0 frame].size.width - [topSubview0 frame].size.width) >= 1.0f)
	{
		if (splitView == topSplit)
		{
			[bottomSubview0 setFrame:[topSubview0 frame]];
			[bottomSubview1 setFrame:[topSubview1 frame]];
		}
		else
		{
			[topSubview0 setFrame:[bottomSubview0 frame]];
			[topSubview1 setFrame:[bottomSubview1 frame]];
		}
	}
}

- (void)collapseSplitView:(NSSplitView *)splitView
{
	[[[splitView subviews] objectAtIndex:0] setFrame:NSZeroRect];
}

- (void)swapCamerasBetweenFirst:(OpenGLSceneView *)first second:(OpenGLSceneView *)second
{
	Camera firstCamera = [first camera];
	Camera secondCamera = [second camera];
	CameraMode firstMode = [first cameraMode];
	CameraMode secondMode = [second cameraMode];
	[second setCameraMode:firstMode];
	[first setCameraMode:secondMode];
	[second setCamera:firstCamera];
	[first setCamera:secondCamera];
}

- (void)toggleOneViewFourView:(id)sender
{
	if (oneView)
	{
		if (oneView != viewPerspective)
		{
			[self swapCamerasBetweenFirst:oneView second:viewPerspective];
		}
		NSRect frame = [viewPerspective frame];
		[[[mainSplit subviews] objectAtIndex:0] setFrame:frame];
		oneView = nil;
		return;
	}
	
	NSWindow *window = [viewPerspective window];
	NSPoint point = [window convertScreenToBase:[NSEvent mouseLocation]];
	
	NSView *hittedView = [[[window contentView] superview] hitTest:point];
	
	for (OpenGLSceneView *view in views)
	{
		if (view == hittedView)
		{
			oneView = view;
			
			[self collapseSplitView:mainSplit];
            [self collapseSplitView:topSplit];
			[self collapseSplitView:bottomSplit];			
			
			if (oneView != viewPerspective)
			{
				[self swapCamerasBetweenFirst:oneView second:viewPerspective];
			}
			return;
		}
	}	
}

@end
