//
//  Mesh2.cpp
//  OpenGLEditor
//
//  Created by Filip Kunc on 19.06.11.
//  For license see LICENSE.TXT
//

#include "Mesh2.h"
#include "TextureCollection.h"

bool Mesh2::_useSoftSelection = false;
bool Mesh2::_selectThrough = false;
float Mesh2::_minimumSelectionWeight = 0.1f;
vector<float> *Mesh2::_selectionWeights = NULL;

vector<float> &Mesh2::selectionWeights()
{
    if (_selectionWeights == NULL)
    {
        _selectionWeights = new vector<float>();
        _selectionWeights->push_back(1.0f);
        _selectionWeights->push_back(0.75f);
        _selectionWeights->push_back(0.5f);
        _selectionWeights->push_back(0.2f);
    }
    return *_selectionWeights;
}

Vector4D generateRandomColor();

Vector4D generateRandomColor()
{
    float hue = (random() % 10) / 10.0f;
    
    NSColor *color = [NSColor colorWithCalibratedHue:hue
                                          saturation:0.5f
                                          brightness:0.6f
                                               alpha:1.0f];
    
    return Vector4D(color.redComponent, color.greenComponent, color.blueComponent, color.alphaComponent);
}

Triangle2 makeTriangle(VertexNode *vertices[], TexCoordNode *texCoords[],
                       unsigned int index0, unsigned int index1, unsigned int index2);

Triangle2 makeTriangle(VertexNode *vertices[], TexCoordNode *texCoords[],
                       unsigned int index0, unsigned int index1, unsigned int index2)
{
    VertexNode *triangleVertices[3];
    triangleVertices[0] = vertices[index0];
    triangleVertices[1] = vertices[index1];
    triangleVertices[2] = vertices[index2];
    TexCoordNode *triangleTexCoords[3];
    triangleTexCoords[0] = texCoords[index0];
    triangleTexCoords[1] = texCoords[index1];
    triangleTexCoords[2] = texCoords[index2];
    return Triangle2(triangleVertices, triangleTexCoords, false);
}

Triangle2 makeQuad(VertexNode *vertices[], TexCoordNode *texCoords[],
                   unsigned int index0, unsigned int index1, unsigned int index2, unsigned int index3);

Triangle2 makeQuad(VertexNode *vertices[], TexCoordNode *texCoords[],
                   unsigned int index0, unsigned int index1, unsigned int index2, unsigned int index3)
{
    VertexNode *triangleVertices[4];
    triangleVertices[0] = vertices[index0];
    triangleVertices[1] = vertices[index1];
    triangleVertices[2] = vertices[index2];
    triangleVertices[3] = vertices[index3];
    TexCoordNode *triangleTexCoords[4];
    triangleTexCoords[0] = texCoords[index0];
    triangleTexCoords[1] = texCoords[index1];
    triangleTexCoords[2] = texCoords[index2];
    triangleTexCoords[3] = texCoords[index3];
    return Triangle2(triangleVertices, triangleTexCoords, true);
}

NSString *Mesh2::descriptionOfMeshType(MeshType meshType)
{
    switch (meshType)
	{
		case MeshType::Cube:
			return @"Cube";
		case MeshType::Cylinder:
			return @"Cylinder";
		case MeshType::Sphere:
			return @"Sphere";
        case MeshType::Plane:
            return @"Plane";
        case MeshType::Icosahedron:
            return @"Icosahedron";
		default:
			return nil;
	}
}

Mesh2::Mesh2()
{
    _selectionMode = MeshSelectionMode::Vertices;
    
    _vboID = 0U;
    _vboGenerated = false;
    
    _isUnwrapped = false;
    
    _texture = NULL;
    
    setColor(generateRandomColor());
}

Mesh2::Mesh2(MemoryReadStream *stream, TextureCollection &textures)
{
	_selectionMode = MeshSelectionMode::Vertices;
    
    _vboID = 0U;
    _vboGenerated = false;
    
    _isUnwrapped = false;
    
    _texture = NULL;
    
    setColor(generateRandomColor());
    
    const ModelVersion version = (ModelVersion)stream->version();
    
    if (version >= ModelVersion::TextureNames)
    {
        unsigned int textureIndex = stream->read<unsigned int>();
        if (textureIndex < textures.count())
            _texture = textures.textureAtIndex(textureIndex);
        else
            _texture = NULL;
    }
    
    Vector4D color;

    if (version >= ModelVersion::CrossPlatform)
    {
        color.x = stream->read<float>();
        color.y = stream->read<float>();
        color.z = stream->read<float>();
        color.w = stream->read<float>();
    }
    else if (version >= ModelVersion::Colors)
    {
        color = stream->read<Vector4D>();
    }
    else
    {
        color = generateRandomColor();
    }
    
    unsigned int verticesSize = stream->read<unsigned int>();
    unsigned int texCoordsSize = stream->read<unsigned int>();
    unsigned int trianglesSize = stream->read<unsigned int>();
    
    vector<Vector3D> vertices;
    vector<Vector3D> texCoords;
    vector<TriQuad> triangles;
    
    if (version >= ModelVersion::CrossPlatform)
    {
        for (unsigned int i = 0; i < verticesSize; i++)
        {
            Vector3D vertex;
            vertex.x = stream->read<float>();
            vertex.y = stream->read<float>();
            vertex.z = stream->read<float>();
            vertices.push_back(vertex);
        }
        
        for (unsigned int i = 0; i < texCoordsSize; i++)
        {
            Vector3D texCoord;
            texCoord.x = stream->read<float>();
            texCoord.y = stream->read<float>();
            texCoord.z = stream->read<float>();
            texCoords.push_back(texCoord);
        }
        
        for (unsigned int i = 0; i < trianglesSize; i++)
        {
            TriQuad triangle;
            triangle.isQuad = stream->read<bool>();
            unsigned int count = triangle.isQuad ? 4 : 3;
            for (unsigned int j = 0; j < count; j++)
            {
                triangle.vertexIndices[j] = stream->read<unsigned int>();
                triangle.texCoordIndices[j] = stream->read<unsigned int>();
            }
            triangles.push_back(triangle);
        }
    }
    else
    {
        for (unsigned int i = 0; i < verticesSize; i++)
        {
            Vector3D vertex = stream->read<Vector3D>();
            vertices.push_back(vertex);
        }
        
        for (unsigned int i = 0; i < texCoordsSize; i++)
        {
            Vector3D texCoord = stream->read<Vector3D>();
            texCoords.push_back(texCoord);
        }
        
        if (version >= ModelVersion::TriQuads)
        {
            for (unsigned int i = 0; i < trianglesSize; i++)
            {
                TriQuad triangle = stream->read<TriQuad>();
                triangles.push_back(triangle);
            }
        }
        else
        {
            for (unsigned int i = 0; i < trianglesSize; i++)
            {
                Triangle triangle = stream->read<Triangle>();
                TriQuad triQuad;
                triQuad.isQuad = false;
                for (unsigned int j = 0; j < 3; j++)
                {
                    triQuad.vertexIndices[j] = triangle.vertexIndices[j];
                    triQuad.texCoordIndices[j] = triangle.texCoordIndices[j];
                }
                triangles.push_back(triQuad);
            }
        }
    }
    
    this->fromIndexRepresentation(vertices, texCoords, triangles);
    this->setColor(color);
}

void Mesh2::encode(MemoryWriteStream *stream, TextureCollection &textures)
{
    if (_texture == NULL)
        stream->write<unsigned int>(UINT_MAX);
    else
        stream->write<unsigned int>(textures.indexOfTexture(_texture));
    
    stream->write<float>(_color.x);
    stream->write<float>(_color.y);
    stream->write<float>(_color.z);
    stream->write<float>(_color.w);
        
    vector<Vector3D> vertices;
    vector<Vector3D> texCoords;
    vector<TriQuad> triangles;
    
    this->toIndexRepresentation(vertices, texCoords, triangles);
    
    unsigned int vertexCount = static_cast<unsigned int>(vertices.size());
    unsigned int texCoordCount = static_cast<unsigned int>(texCoords.size());
    unsigned int triangleCount = static_cast<unsigned int>(triangles.size());
    
    stream->write<unsigned int>(vertexCount);
    stream->write<unsigned int>(texCoordCount);
    stream->write<unsigned int>(triangleCount);
    
    for (unsigned int i = 0; i < vertexCount; i++)
    {
        const Vector3D &v = vertices[i];
        stream->write<float>(v.x);
        stream->write<float>(v.y);
        stream->write<float>(v.z);        
    }

    for (unsigned int i = 0; i < texCoordCount; i++)
    {
        const Vector3D &v = texCoords[i];
        stream->write<float>(v.x);
        stream->write<float>(v.y);
        stream->write<float>(v.z);
    }
	
	for (unsigned int i = 0; i < triangleCount; i++)
    {
        const TriQuad &t = triangles[i];
        stream->write<bool>(t.isQuad);
        unsigned int count = t.isQuad ? 4 : 3;
        for (unsigned int j = 0; j < count; j++)
        {
            stream->write(t.vertexIndices[j]);
            stream->write(t.texCoordIndices[j]);
        }
    }
}

void Mesh2::setColor(Vector4D color)
{
    _color = color;
    for (int i = 0; i < 4; i++)
        _colorComponents[i] = _color[i];
    
    resetTriangleCache();    
}

Mesh2::~Mesh2()
{
    resetTriangleCache();
}

void Mesh2::resetAlgorithmData()
{
    for (VertexNode *vertexNode = _vertices.begin(), *vertexEnd = _vertices.end(); vertexNode != vertexEnd; vertexNode = vertexNode->next())
        vertexNode->algorithmData.clear();
    
    for (TexCoordNode *texCoordNode = _texCoords.begin(), *texCoordEnd = _texCoords.end(); texCoordNode != texCoordEnd; texCoordNode = texCoordNode->next())
        texCoordNode->algorithmData.clear();
}

void Mesh2::setSelectionMode(MeshSelectionMode value)
{
    resetEdgeCache();
    
    _selectionMode = value;
    _cachedVertexSelection.clear();
    _cachedTriangleSelection.clear();
    _cachedVertexEdgeSelection.clear();
    _cachedTexCoordSelection.clear();
    _cachedTexCoordEdgeSelection.clear();
    
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
        {
            for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
                _cachedVertexSelection.push_back(node);
            
            for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
                _cachedTexCoordSelection.push_back(node);            
        } break;
        case MeshSelectionMode::Triangles:
        {
            for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
                node->data().selected = false;
            
            for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
                node->data().selected = false;
            
            for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
            {
                _cachedTriangleSelection.push_back(node);
                
                Triangle2 &triangle = node->data();
                if (triangle.selected)
                {
                    for (unsigned int i = 0; i < triangle.count(); i++)
                    {
                        triangle.vertex(i)->data().selected = true;
                        triangle.texCoord(i)->data().selected = true;
                    }
                }
            }
        } break;
        case MeshSelectionMode::Edges:
        {
            for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
                node->data().selected = false;
            
            for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
                node->data().selected = false;
            
            if (_isUnwrapped)
            {
                for (TexCoordEdgeNode *node = _texCoordEdges.begin(), *end = _texCoordEdges.end(); node != end; node = node->next())
                {
                    _cachedTexCoordEdgeSelection.push_back(node);
                    
                    TexCoordEdge &edge = node->data();
                    if (edge.selected)
                    {
                        for (unsigned int i = 0; i < 2; i++)
                            edge.texCoord(i)->data().selected = true;
                    }
                }
            }
            else
            {
                for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
                {
                    _cachedVertexEdgeSelection.push_back(node);
                    
                    VertexEdge &edge = node->data();
                    if (edge.selected)
                    {
                        for (unsigned int i = 0; i < 2; i++)
                            edge.vertex(i)->data().selected = true;
                    }
                }
            }
        } break;
        default:
            break;
    }
}

unsigned int Mesh2::selectedCount() const
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
            if (_isUnwrapped)
                return static_cast<unsigned int>(_cachedTexCoordSelection.size());
            return static_cast<unsigned int>(_cachedVertexSelection.size());
        case MeshSelectionMode::Triangles:
            return static_cast<unsigned int>(_cachedTriangleSelection.size());
        case MeshSelectionMode::Edges:
            if (_isUnwrapped)
                return static_cast<unsigned int>(_cachedTexCoordEdgeSelection.size());
            return static_cast<unsigned int>(_cachedVertexEdgeSelection.size());
        default:
            return 0;
    }
}

bool Mesh2::isSelectedAtIndex(unsigned int index) const
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
            if (_isUnwrapped)
                return _cachedTexCoordSelection.at(index)->data().selected;
            return _cachedVertexSelection.at(index)->data().selected;
        case MeshSelectionMode::Triangles:
            return _cachedTriangleSelection.at(index)->data().selected;
        case MeshSelectionMode::Edges:
            if (_isUnwrapped)
                return _cachedTexCoordEdgeSelection.at(index)->data().selected;
            return _cachedVertexEdgeSelection.at(index)->data().selected;
        default:
            return false;
    }
}

void Mesh2::setSelectedAtIndex(bool selected, unsigned int index)
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
            if (_isUnwrapped)
                _cachedTexCoordSelection.at(index)->data().selected = selected;
            else
                _cachedVertexSelection.at(index)->data().selected = selected;
            break;
        case MeshSelectionMode::Triangles:
        {
            Triangle2 &triangle = _cachedTriangleSelection.at(index)->data();
            triangle.selected = selected;
            for (unsigned int i = 0; i < triangle.count(); i++)
            {
                triangle.vertex(i)->data().selected = selected;
                triangle.texCoord(i)->data().selected = selected;
            }                
        } break;
        case MeshSelectionMode::Edges:
        {
            if (_isUnwrapped)
            {
                TexCoordEdge &edge = _cachedTexCoordEdgeSelection.at(index)->data();
                edge.selected = selected;
                for (unsigned int i = 0; i < 2; i++)
                    edge.texCoord(i)->data().selected = selected;
            }
            else                
            {
                VertexEdge &edge = _cachedVertexEdgeSelection.at(index)->data();
                edge.selected = selected;
                for (unsigned int i = 0; i < 2; i++)
                    edge.vertex(i)->data().selected = selected;
            }
        } break;
        default:
            break;
    }
}

void Mesh2::expandSelectionFromIndex(unsigned int index, bool invert)
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Edges:
        {
            VertexEdge &edge = _cachedVertexEdgeSelection.at(index)->data();

            if (invert)
                edge.selectEdgesInQuadLoop();
            else
                edge.selectEdgesInEdgeLoop();
            
            setSelectionMode(_selectionMode);
        } break;
        default:
            break;
    }
}

void Mesh2::getSelectionCenterRotationScale(Vector3D &center, Quaternion &rotation, Vector3D &scale)
{
    center = Vector3D();
	rotation = Quaternion();
	scale = Vector3D(1, 1, 1);
    
	unsigned int selectedCount = 0;
    
    if (_isUnwrapped)
    {
        for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
        {
            if (node->data().selected)
            {
                center += node->data().position;
                selectedCount++;
            }
        }
    }
    else
    {
        for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
        {
            if (node->data().selected)
            {
                center += node->data().position;
                selectedCount++;
            }
        }        
    }
	if (selectedCount > 0)
		center /= (float)selectedCount;
}

void Mesh2::transformAll(const Matrix4x4 &matrix)
{
    resetTriangleCache();
    
    if (_isUnwrapped)
    {
        for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
        {
            Vector3D &v = node->data().position;
            v = matrix.Transform(v);
        }
    }
    else
    {
        for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
        {
            Vector3D &v = node->data().position;
            v = matrix.Transform(v);
        }
    }
    
    setSelectionMode(_selectionMode);
}

void Mesh2::transformSelected(const Matrix4x4 &matrix)
{
    if (_isUnwrapped)
    {
        resetTriangleCache();
        
        for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
        {
            if (node->data().selected)
            {
                Vector3D &v = node->data().position;
                v = matrix.Transform(v);
            }
        }
    }
    else
    {
        vector<VertexNode *> affectedVertices;
        
        if (_useSoftSelection)
        {
            resetTriangleCache();
            
            for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
            {
                if (node->selectionWeight > _minimumSelectionWeight)
                {
                    Vector3D &v = node->data().position;
                    v = v.Lerp(matrix.Transform(v), node->selectionWeight);
                }
            }
        }
        else
        {
            for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
            {
                if (node->data().selected)
                {
                    Vector3D &v = node->data().position;
                    v = matrix.Transform(v);
                    affectedVertices.push_back(node);
                }
            }
        }
        
        updateTriangleAndEdgeCache(affectedVertices);
    }
}

void Mesh2::fastMergeSelectedVertices()
{
    Vector3D center = Vector3D();
    
    SimpleList<VertexNode *> selectedNodes;
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (node->data().selected)
        {
            selectedNodes.add(node);
            center += node->data().position;
        }
    }
    
    if (selectedNodes.count() < 2)
        return;
    
    center /= (float)selectedNodes.count();
    
    VertexNode *centerNode = _vertices.add(center);
    
    for (SimpleNode<VertexNode *> *node = selectedNodes.begin(), *end = selectedNodes.end(); node != end; node = node->next())
    {
        node->data()->replaceVertex(centerNode);        
    }    
}

void Mesh2::fastMergeSelectedTexCoords()
{
    Vector3D center = Vector3D();
    
    SimpleList<TexCoordNode *> selectedNodes;
    
    for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
    {
        if (node->data().selected)
        {
            selectedNodes.add(node);
            center += node->data().position;
        }
    }
    
    if (selectedNodes.count() < 2)
        return;
    
    center /= (float)selectedNodes.count();
    
    TexCoordNode *centerNode = _texCoords.add(center);
    
    for (SimpleNode<TexCoordNode *> *node = selectedNodes.begin(), *end = selectedNodes.end(); node != end; node = node->next())
    {
        node->data()->replaceVertex(centerNode);
    }
}

void Mesh2::removeDegeneratedTriangles()
{
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        if (node->data().isDegeneratedAfterCollapseToTriangle())
            _triangles.remove(node);
    }
}

void Mesh2::removeNonUsedVertices()
{
    resetTriangleCache();
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (!node->isUsed())
            _vertices.remove(node);
    }
}

void Mesh2::removeNonUsedTexCoords()
{
    resetTriangleCache();
    
    for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
    {
        if (!node->isUsed())
            _texCoords.remove(node);
    }
}

void Mesh2::mergeSelectedVertices()
{
    resetTriangleCache();
    
    if (_isUnwrapped)
        fastMergeSelectedTexCoords();
    else
        fastMergeSelectedVertices();
    
    removeDegeneratedTriangles();
    removeNonUsedVertices();
    removeNonUsedTexCoords();
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::removeSelectedVertices()
{
    resetTriangleCache();
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            _vertices.remove(node);
    }
    
    removeDegeneratedTriangles();
    removeNonUsedVertices();
    removeNonUsedTexCoords();
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::removeSelectedTriangles()
{
    resetTriangleCache();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            _triangles.remove(node);
    }
    
    removeNonUsedVertices();
    removeNonUsedTexCoords();
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::removeSelectedEdges()
{
    resetTriangleCache();
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        VertexEdge &edge = node->data();
        if (edge.selected)
        {
            TriangleNode *t0, *t1;
            
            if ((t0 = edge.triangle(0)) && (t1 = edge.triangle(1)) &&
                !t0->data().isQuad() && !t1->data().isQuad())
            {
                VertexNode *v0 = t0->data().vertexNotInEdge(&edge);
                VertexNode *v1 = t1->data().vertexNotInEdge(&edge);
                
                VertexNode *v2 = edge.vertex(0);
                VertexNode *v3 = edge.vertex(1);
                
                TriangleNode *quad = addQuad(v3, v0, v2, v1);
                makeEdges(quad);
                
                _triangles.remove(t0);
                _triangles.remove(t1);
            }
            
            _vertexEdges.remove(node);
        }
    }
    
    removeDegeneratedTriangles();
    removeNonUsedVertices();
    removeNonUsedTexCoords();

    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::removeSelected()
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Triangles:
            removeSelectedTriangles();
            break;
        case MeshSelectionMode::Vertices:
            removeSelectedVertices();
            break;
        case MeshSelectionMode::Edges:
            removeSelectedEdges();
            break;
        default:
            break;
    }    
}

void Mesh2::mergeSelected()
{
    switch (_selectionMode)
	{
		case MeshSelectionMode::Vertices:
			mergeSelectedVertices();
			break;
		default:
			break;
	}
}

void Mesh2::triangulate()
{
    resetTriangleCache();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 quad = node->data();
        if (quad.isQuad())
        {
            Triangle2 triangle[2];
            
            unsigned int v = 0;
            for (unsigned int i = 0; i < 2; i++)
            {
                for (unsigned int j = 0; j < 3; j++)
                {
                    unsigned int index = Triangle2::twoTriIndices[v];
                    v++;
                    triangle[i].setVertex(j, quad.vertex(index));
                    triangle[i].setTexCoord(j, quad.texCoord(index));
                }
                
                _triangles.add(triangle[i]);
            }
            
            _triangles.remove(node);
        }
    }
    
    makeEdges();
	
    setSelectionMode(_selectionMode);
}

void Mesh2::triangulateSelectedQuads()
{
    resetTriangleCache();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 quad = node->data();
        if (quad.isQuad() && quad.selected)
        {
            Triangle2 triangle[2];
            
            unsigned int v = 0;
            for (unsigned int i = 0; i < 2; i++)
            {
                for (unsigned int j = 0; j < 3; j++)
                {
                    unsigned int index = Triangle2::twoTriIndices[v];
                    v++;
                    triangle[i].setVertex(j, quad.vertex(index));
                    triangle[i].setTexCoord(j, quad.texCoord(index));
                }
                
                _triangles.add(triangle[i]);
            }
            
            _triangles.remove(node);
        }
    }
    
    makeEdges();
	
    setSelectionMode(_selectionMode);
}

void Mesh2::halfEdges()
{
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        Vector3D v1 = node->data().vertex(0)->data().position;
        Vector3D v2 = node->data().vertex(1)->data().position;
        
        Vector3D edgeVertex;
        
        // boundary
        if (node->data().triangle(0) == NULL || node->data().triangle(1) == NULL)
        {
            edgeVertex = (v1 + v2) / 2.0f;
        }
        else
        {
            Triangle2 &t0 = node->data().triangle(0)->data();
            Triangle2 &t1 = node->data().triangle(1)->data();
            
            VertexNode *vn0 = t0.vertexNotInEdge(&node->data());
            VertexNode *vn1 = t1.vertexNotInEdge(&node->data());
            
            // degenerated triangles
            if (vn0 == NULL || vn1 == NULL)
            {
                edgeVertex = (v1 + v2) / 2.0f;
            }
            else
            {
                Vector3D v3 = vn0->data().position;
                Vector3D v4 = vn1->data().position;
                
                edgeVertex = 3.0f * (v1 + v2) / 8.0f + 1.0f * (v3 + v4) / 8.0f;
            }
        }
        
        node->data().half = _vertices.add(edgeVertex);
    }
    
    for (TexCoordEdgeNode *node = _texCoordEdges.begin(), *end = _texCoordEdges.end(); node != end; node = node->next())
    {
        Vector3D t1 = node->data().texCoord(0)->data().position;
        Vector3D t2 = node->data().texCoord(1)->data().position;
        
        Vector3D edgeTexCoord = (t1 + t2) / 2.0f;
        
        node->data().half = _texCoords.add(edgeTexCoord);
    }
}

void Mesh2::repositionVertices(unsigned int vertexCount)
{
    resetAlgorithmData();
    
    vector<Vector3D> tempVertices;
    
    unsigned int index = 0;
    
    for (VertexNode *node = _vertices.begin(); index < vertexCount; node = node->next())
    {
        node->algorithmData.index = index;
        index++;
        
        float beta, n;
        
        n = (float)node->_edges.count();
        beta = 3.0f + 2.0f * cosf(FLOAT_PI * 2.0f / n);
        beta = 5.0f / 8.0f - (beta * beta) / 64.0f;
        
        Vector3D finalPosition = (1.0f - beta) * node->data().position;
        
        float bon = beta / n;
        
        for (Vertex2VEdgeNode *vertexEdgeNode = node->_edges.begin(), *endVertexEdgeNode = node->_edges.end(); vertexEdgeNode != endVertexEdgeNode; vertexEdgeNode = vertexEdgeNode->next())
        {
            VertexEdge &edge = vertexEdgeNode->data()->data();
            VertexNode *oppositeVertex = edge.opposite(node);
            
            finalPosition += oppositeVertex->data().position * bon;
        }
        
        tempVertices.push_back(finalPosition);
    }
    
    index = 0;
    
    for (VertexNode *node = _vertices.begin(); index < vertexCount; node = node->next())
    {
        node->data().position = tempVertices[(unsigned int)node->algorithmData.index];
        index++;
    }
}

void Mesh2::makeSubdividedTriangles()
{
    VertexNode *vertices[6];
    TexCoordNode *texCoords[6];
    FPList<TriangleNode, Triangle2> subdivided;
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        for (unsigned int i = 0; i < 3; i++)
        {
            vertices[i] = node->data().vertex(i);
            vertices[i + 3] = node->data().vertexEdge(i)->data().half;
            
            texCoords[i] = node->data().texCoord(i);
            texCoords[i + 3] = node->data().texCoordEdge(i)->data().half;
        }
        
        /*
               2
              /\
             /  \
          *5/____\*4
           /\    /\
          /  \  /  \
         /____\/____\
         0    *3     1
         
         */
        
        subdivided.add(makeTriangle(vertices, texCoords, 0, 3, 5));
        subdivided.add(makeTriangle(vertices, texCoords, 3, 1, 4));
        subdivided.add(makeTriangle(vertices, texCoords, 5, 4, 2));
        subdivided.add(makeTriangle(vertices, texCoords, 3, 4, 5));
    }
    
    _triangles.moveFrom(subdivided);
}

void Mesh2::loopSubdivision()
{
    triangulate();
    
    resetTriangleCache();
    
    unsigned int vertexCount = _vertices.count();
    
    halfEdges();
    repositionVertices(vertexCount);
    makeSubdividedTriangles();
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::detachSelectedVertices()
{
    resetTriangleCache();
    
    if (_isUnwrapped)
    {
        for (TexCoordNode *texCoordNode = _texCoords.begin(), *texCoordEnd = _texCoords.end(); texCoordNode != texCoordEnd; texCoordNode = texCoordNode->next())
        {
            if (!texCoordNode->data().selected)
                continue;
            
            texCoordNode->data().selected = false;
            
            for (VertexTriangleNode 
                 *triangleNode = texCoordNode->_triangles.begin(), 
                 *triangleEnd = texCoordNode->_triangles.end(); 
                 triangleNode != triangleEnd; 
                 triangleNode = triangleNode->next())
            {
                triangleNode->data()->replaceTexCoord(texCoordNode, _texCoords.add(texCoordNode->data()));
            }
            
            _texCoords.remove(texCoordNode);
        }
    }
    else
    {
        for (VertexNode *vertexNode = _vertices.begin(), *vertexEnd = _vertices.end(); vertexNode != vertexEnd; vertexNode = vertexNode->next())
        {
            if (!vertexNode->data().selected)
                continue;
            
            vertexNode->data().selected = false;
            
            for (VertexTriangleNode
                 *triangleNode = vertexNode->_triangles.begin(), 
                 *triangleEnd = vertexNode->_triangles.end(); 
                 triangleNode != triangleEnd; 
                 triangleNode = triangleNode->next())
            {
                triangleNode->data()->replaceVertex(vertexNode, _vertices.add(vertexNode->data()));
            }
            
            _vertices.remove(vertexNode);
        }
    }
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::detachSelectedTriangles()
{
    resetTriangleCache();
    
    if (_isUnwrapped)
    {
        for (TexCoordNode *texCoordNode = _texCoords.begin(), *texCoordEnd = _texCoords.end(); texCoordNode != texCoordEnd; texCoordNode = texCoordNode->next())
        {
            TexCoordNode *newTexCoord = NULL;
            
            for (VertexTriangleNode
                 *triangleNode = texCoordNode->_triangles.begin(), 
                 *triangleEnd = texCoordNode->_triangles.end(); 
                 triangleNode != triangleEnd; 
                 triangleNode = triangleNode->next())
            {
                if (triangleNode->data()->data().selected)
                {
                    if (newTexCoord == NULL)
                        newTexCoord = _texCoords.add(texCoordNode->data());
                    
                    triangleNode->data()->replaceTexCoord(texCoordNode, newTexCoord);
                    texCoordNode->_triangles.remove(triangleNode);
                }
            }
            
            if (!texCoordNode->isUsed())
                _texCoords.remove(texCoordNode);
        }
    }
    else
    {    
        for (VertexNode *vertexNode = _vertices.begin(), *vertexEnd = _vertices.end(); vertexNode != vertexEnd; vertexNode = vertexNode->next())
        {
            VertexNode *newVertex = NULL;
            
            for (VertexTriangleNode
                 *triangleNode = vertexNode->_triangles.begin(), 
                 *triangleEnd = vertexNode->_triangles.end(); 
                 triangleNode != triangleEnd; 
                 triangleNode = triangleNode->next())
            {
                if (triangleNode->data()->data().selected)
                {
                    if (newVertex == NULL)
                        newVertex = _vertices.add(vertexNode->data());
                        
                    triangleNode->data()->replaceVertex(vertexNode, newVertex);
                    vertexNode->_triangles.remove(triangleNode);
                }
            }
            
            if (!vertexNode->isUsed())
                _vertices.remove(vertexNode);
        }
    }
        
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::splitSelectedTriangles()
{
    resetTriangleCache();
    
    VertexNode *v[9];
    TexCoordNode *t[9];
    
    FPList<TriangleNode, Triangle2> subdivided;
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        node->data().half = NULL;
    }
    
    for (TexCoordEdgeNode *node = _texCoordEdges.begin(), *end = _texCoordEdges.end(); node != end; node = node->next())
    {
        node->data().half = NULL;
    }
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 &triQuad = node->data();
        
        if (!triQuad.selected)
        {
            subdivided.add(triQuad);
            continue;
        }
        
        for (unsigned int j = 0; j < triQuad.count(); j++)
        {
            VertexEdge &edge = node->data().vertexEdge(j)->data();
            
            if (edge.half == NULL)
            {
                Vector3D v0 = edge.vertex(0)->data().position;
                Vector3D v1 = edge.vertex(1)->data().position;
                
                Vector3D edgeVertex = (v0 + v1) / 2.0f;
                
                edge.half = _vertices.add(edgeVertex);
            }
            
            v[j] = node->data().vertex(j);
            v[j + triQuad.count()] = edge.half;
        }
        
        for (unsigned int j = 0; j < triQuad.count(); j++)
        {
            TexCoordEdge &edge = node->data().texCoordEdge(j)->data();
            
            if (edge.half == NULL)
            {
                Vector3D t0 = edge.texCoord(0)->data().position;
                Vector3D t1 = edge.texCoord(1)->data().position;
                
                Vector3D edgeTexCoord = (t0 + t1) / 2.0f;
                
                edge.half = _texCoords.add(edgeTexCoord);
            }
            
            t[j] = node->data().texCoord(j);
            t[j + triQuad.count()] = edge.half;
        }
        
        if (triQuad.isQuad())
        {
            /*      
                3----(6)----2
                |     |     |
                |     |     |
               (7)---[8]---(5)
                |     |     |
                |     |     |
                0----(4)----1
    
            */
            
            v[8] = _vertices.add((v[7]->data().position + v[5]->data().position) / 2.0f);
            t[8] = _texCoords.add((t[7]->data().position + t[5]->data().position) / 2.0f);
            
            subdivided.add(makeQuad(v, t, 0, 4, 8, 7));
            subdivided.add(makeQuad(v, t, 4, 1, 5, 8));
            subdivided.add(makeQuad(v, t, 8, 5, 2, 6));
            subdivided.add(makeQuad(v, t, 7, 8, 6, 3));
        }
        else
        {
            /*
                   2
                  /\
                 /  \
              *5/____\*4
               /\    /\
              /  \  /  \
             /____\/____\
             0    *3     1
             
            */
            
            subdivided.add(makeTriangle(v, t, 0, 3, 5));
            subdivided.add(makeTriangle(v, t, 3, 1, 4));
            subdivided.add(makeTriangle(v, t, 5, 4, 2));
            subdivided.add(makeTriangle(v, t, 3, 4, 5));
        }
    }
    
    _triangles.moveFrom(subdivided);
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::splitSelectedEdges()
{
    resetTriangleCache();
    
    for (VertexEdgeNode *edgeNode = _vertexEdges.begin(), *end = _vertexEdges.end(); edgeNode != end; edgeNode = edgeNode->next())
    {
        VertexEdge &vertexEdge = edgeNode->data();
        if (!vertexEdge.selected)
            continue;
        
        if (vertexEdge.half == NULL)
        {
            Vector3D v0 = vertexEdge.vertex(0)->data().position;
            Vector3D v1 = vertexEdge.vertex(1)->data().position;
            
            Vector3D edgeVertex = (v0 + v1) / 2.0f;
            
            vertexEdge.half = _vertices.add(edgeVertex);
        }
    }
    
    VertexNode *v[4];
    
    for (VertexEdgeNode *edgeNode = _vertexEdges.begin(), *end = _vertexEdges.end(); edgeNode != end; edgeNode = edgeNode->next())
    {
        VertexEdge &vertexEdge = edgeNode->data();
        if (!vertexEdge.selected)
            continue;
        
        for (unsigned int i = 0; i < 2; i++)
        {
            TriangleNode *triangleNode = vertexEdge.triangle(i);
            if (triangleNode == NULL)
                continue;
            
            Triangle2 &triQuad = triangleNode->data();
            
            VertexEdgeNode *secondEdgeNode = triQuad.findSecondSplitEdgeNode(vertexEdge);
            if (secondEdgeNode == NULL)
                continue;
            
            VertexEdge &secondEdge = secondEdgeNode->data();
            
            if (triQuad.isQuad())
            {
                for (unsigned int k = 0; k < 2; k++)
                {
                    v[0] = secondEdge.half;
                    v[1] = vertexEdge.half;
                    
                    v[2] = vertexEdge.vertex(k);
                    
                    if (v[2]->sharedEdge(secondEdge.vertex(0)))
                        v[3] = secondEdge.vertex(0);
                    else
                        v[3] = secondEdge.vertex(1);
                    
                    if (triQuad.shouldSwapVertices(v[2], v[3]))
                    {
                        swap(v[2], v[3]);
                        swap(v[1], v[0]);
                    }
                    
                    _triangles.add(Triangle2(v, true));
                }                                
            }
            else
            {
                v[0] = secondEdge.half;
                v[1] = vertexEdge.half;
                v[2] = vertexEdge.sharedVertex(secondEdge);
                
                VertexNode *opposite0 = secondEdge.opposite(v[2]);
                VertexNode *opposite1 = vertexEdge.opposite(v[2]);
                
                if (triQuad.shouldSwapVertices(opposite0, opposite1))
                {
                    swap(opposite0, opposite1);
                    swap(v[0], v[1]);
                }
                
                _triangles.add(Triangle2(v, false));
                
                v[2] = opposite1;
                v[3] = opposite0;
                
                swap(v[0], v[2]);
                
                _triangles.add(Triangle2(v, true));                
            }
            
            _triangles.remove(triangleNode);
        }
    }
    
    removeNonUsedVertices();    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::splitSelected()
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Triangles:
            splitSelectedTriangles();
            break;
        case MeshSelectionMode::Edges:
            splitSelectedEdges();
            break;
        default:
            break;
    }
}

void Mesh2::detachSelected()
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
            detachSelectedVertices();
            break;
        case MeshSelectionMode::Triangles:
            detachSelectedTriangles();
            break;
        default:
            break;
    }
}

void Mesh2::duplicateSelectedTriangles()
{
    resetTriangleCache();
    resetAlgorithmData();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 &triQuad = node->data();
        
        if (!triQuad.selected)
            continue;
        
        VertexNode *duplicatedVertices[4];
        TexCoordNode *duplicatedTexCoords[4];
        
        for (unsigned int i = 0; i < triQuad.count(); i++)
        {
            VertexNode *originalVertex = triQuad.vertex(i);
            TexCoordNode *originalTexCoord = triQuad.texCoord(i);
            duplicatedVertices[i] = duplicateVertex(originalVertex);
            duplicatedTexCoords[i] = duplicateVertex(originalTexCoord);
        }
        
        node->data().selected = false;
        
        TriangleNode *newTriangle = _triangles.add(Triangle2(duplicatedVertices, duplicatedTexCoords, triQuad.isQuad()));
        newTriangle->data().selected = true;
    }
        
    makeEdges();
    
    setSelectionMode(_selectionMode);    
}

void Mesh2::flipSelectedTriangles()
{
    resetTriangleCache();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            node->data().flip();
    }
}

void Mesh2::turnSelectedEdges()
{
    resetTriangleCache();
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            node->data().turn();
    }
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::flipSelected()
{
    switch (_selectionMode) 
    {
        case MeshSelectionMode::Triangles:
            flipSelectedTriangles();
            break;
        case MeshSelectionMode::Edges:
            turnSelectedEdges();
            break;
        default:
            break;
    }
}

void Mesh2::flipAllTriangles()
{
    resetTriangleCache();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        node->data().flip();
    }
}

void Mesh2::extrudeSelected()
{
    switch (_selectionMode)
    {
        case MeshSelectionMode::Triangles:
            extrudeSelectedTriangles();
            break;
        case MeshSelectionMode::Edges:
            extrudeSelectedEdges();
            break;
        default:
            break;
    }
}

void Mesh2::extrudeSelectedEdges()
{
    vector<VertexNode *> extrudedVertices;
    
    resetTriangleCache();
    resetAlgorithmData();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        node->data().selected = false;
    }
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        VertexEdge &vertexEdge = node->data();
        
        if (!vertexEdge.selected)
            continue;
        
        if (vertexEdge.isNotShared())
        {
            Triangle2 &triQuad = vertexEdge.triangle(0)->data();
            triQuad.selected = true;
            
            VertexNode *original0 = vertexEdge.vertex(0);
            VertexNode *original1 = vertexEdge.vertex(1);
            
            if (triQuad.shouldSwapVertices(original0, original1))
                swap(original0, original1);
            
            VertexNode *extruded0 = duplicateVertex(original0);
            VertexNode *extruded1 = duplicateVertex(original1);
            
            addQuad(original0, original1, extruded1, extruded0);
            
            extrudedVertices.push_back(original0);
            extrudedVertices.push_back(original1);
        }
        
        vertexEdge.selected = false;
    }
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (node->algorithmData.duplicatePair != NULL)
            node->replaceVertexInSelectedTriangles(node->algorithmData.duplicatePair);
    }
    
    makeEdges();
    
    for (unsigned int i = 0; i < extrudedVertices.size(); i += 2)
    {
        VertexEdgeNode *node = extrudedVertices[i]->sharedEdge(extrudedVertices[i + 1]);
        node->data().selected = true;
    }
    
    setSelectionMode(_selectionMode);
}

void Mesh2::extrudeSelectedTriangles()
{
    resetTriangleCache();
    resetAlgorithmData();
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 &triQuad = node->data();
        
        if (!triQuad.selected)
            continue;
        
        for (unsigned int i = 0; i < triQuad.count(); i++)
        {
            VertexEdge &vertexEdge = node->data().vertexEdge(i)->data();
            
            if (vertexEdge.isNotShared())
            {
                VertexNode *original0 = vertexEdge.vertex(0);
                VertexNode *original1 = vertexEdge.vertex(1);
                
                if (triQuad.shouldSwapVertices(original0, original1))
                    swap(original0, original1);
                
                VertexNode *extruded0 = duplicateVertex(original0);
                VertexNode *extruded1 = duplicateVertex(original1);
                
                addQuad(original0, original1, extruded1, extruded0);
            }
        }
    }
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (node->algorithmData.duplicatePair != NULL)
            node->replaceVertexInSelectedTriangles(node->algorithmData.duplicatePair);
    }
        
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::merge(Mesh2 *mesh)
{
    vector<Vector3D> thisVertices;
    vector<Vector3D> thisTexCoords;
    vector<TriQuad> thisTriangles;
    
    this->toIndexRepresentation(thisVertices, thisTexCoords, thisTriangles);
    
    vector<Vector3D> otherVertices;
    vector<Vector3D> otherTexCoords;
    vector<TriQuad> otherTriangles;
    
    mesh->toIndexRepresentation(otherVertices, otherTexCoords, otherTriangles);
    
    vector<Vector3D> mergedVertices;
    vector<Vector3D> mergedTexCoords;
    vector<TriQuad> mergedTriangles;
    
    for (unsigned int i = 0; i < thisVertices.size(); i++)
        mergedVertices.push_back(thisVertices[i]);
    
    for (unsigned int i = 0; i < otherVertices.size(); i++)
        mergedVertices.push_back(otherVertices[i]);
    
    for (unsigned int i = 0; i < thisTexCoords.size(); i++)
        mergedTexCoords.push_back(thisTexCoords[i]);
    
    for (unsigned int i = 0; i < otherTexCoords.size(); i++)
        mergedTexCoords.push_back(otherTexCoords[i]);
    
    for (unsigned int i = 0; i < thisTriangles.size(); i++)
        mergedTriangles.push_back(thisTriangles[i]);
    
    for (unsigned int i = 0; i < otherTriangles.size(); i++)
    {
        TriQuad triangle = otherTriangles[i];
        for (unsigned int j = 0; j < 4; j++)
        {
            triangle.vertexIndices[j] += thisVertices.size();
            triangle.texCoordIndices[j] += thisTexCoords.size();
        }
        mergedTriangles.push_back(triangle);
    }
    
    this->fromIndexRepresentation(mergedVertices, mergedTexCoords, mergedTriangles);     
}

void Mesh2::computeSoftSelection()
{
    if (!_useSoftSelection)
        return;
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
        node->selectionWeight = 0.0f;
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
        node->selectionWeight = 0.0f;
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
        node->selectionWeight = 0.0f;
    
    switch (_selectionMode)
    {
        case MeshSelectionMode::Vertices:
            computeSoftSelectionVertices();
            break;
        case MeshSelectionMode::Triangles:
            computeSoftSelectionTriangles();
            break;
        case MeshSelectionMode::Edges:
            computeSoftSelectionEdges();
            break;
    }
}

void Mesh2::computeSoftSelectionVertices()
{
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            node->softSelect(*_selectionWeights);
    }
}

void Mesh2::computeSoftSelectionTriangles()
{
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            node->softSelect(*_selectionWeights);
    }
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        unsigned int count = node->data().count();
        for (unsigned int i = 0; i < count; i++)
        {
            VertexNode *vertexNode = node->data().vertex(i);
            if (vertexNode->selectionWeight < node->selectionWeight)
                vertexNode->selectionWeight = node->selectionWeight;
        }
    }
}

void Mesh2::computeSoftSelectionEdges()
{
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        if (node->data().selected)
            node->softSelect(*_selectionWeights);
    }
    
    for (VertexEdgeNode *node = _vertexEdges.begin(), *end = _vertexEdges.end(); node != end; node = node->next())
    {
        for (unsigned int i = 0; i < 2; i++)
        {
            VertexNode *vertexNode = node->data().vertex(i);
            if (vertexNode->selectionWeight < node->selectionWeight)
                vertexNode->selectionWeight = node->selectionWeight;
        }
    }
}
