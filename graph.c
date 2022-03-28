#include "defines.h"

#define STR_BUFF_SIZE 512
#define BUFF_SIZE 64000

typedef struct FaceIndex {
	int vertex, uv;
} FaceIndex;

int XLoadSceneGraph(FILE *file, XSceneGraph *scene_graph, char *path_mtl) {
	char buff[STR_BUFF_SIZE];
	char *token;
	
	int object_start_vertices[BUFF_SIZE];	/* numbers of start vertices of each object */
	int object_start_uvs[BUFF_SIZE];	/* numbers of start uv coordinates of each object */
	int object_start_faces[BUFF_SIZE];	/* numbers of start faces of each object */
	char *object_names[BUFF_SIZE];
	int obj_index = 0;
	
	char *material_names[BUFF_SIZE];
	int material_to_object_indexes[BUFF_SIZE];
	int mtl_index = 0;
	
	XPolyGraph *graph; /* current graph */

	XVertex *vertices_buff[BUFF_SIZE];
	int vert_index = 0;
	
	XUVCoordinate *uvs_buff[BUFF_SIZE];
	int uv_index = 0;

	FaceIndex *faces_buff[BUFF_SIZE];	/* face index buffer */
	int faces_num_indexes[BUFF_SIZE];	/* numbers of vertices of each face*/
	int face_index = 0;

	FaceIndex face_indexes[BUFF_SIZE];	/* temporary index buffer for one face */

	/* .obj format parser */
	while ( 1 ) {
		fgets(buff, STR_BUFF_SIZE, file);
		if (feof(file))
			break;

		token = strtok(buff, " ");
		if (strcmp(token, "mtllib") == 0) {
			token = strtok(NULL, "\n");
			char filename[STR_BUFF_SIZE];
			strcpy(filename, path_mtl);
			strcat(filename, token);
			FILE *file;
			if ((file = fopen(filename, "r")) == NULL) {
				fprintf(stderr, "Can not open material library file!\n");
				exit(1);
			}
			XLoadMTLLib(file, scene_graph);
			fclose(file);
		}
		if (strcmp(token, "o") == 0) {
			token = strtok(NULL, "\n");
			object_names[obj_index] = (char*)malloc((strlen(token) + 1) * sizeof(char));
			strcpy(object_names[obj_index], token);
			object_start_vertices[obj_index] = vert_index;
			object_start_uvs[obj_index] = uv_index;
			object_start_faces[obj_index] = face_index;
			obj_index++;
		}
		if (strcmp(token, "v") == 0) {
			vertices_buff[vert_index] = (XVertex*)malloc(sizeof(XVertex));
			vertices_buff[vert_index]->x = atof(strtok(NULL, " ")) * scene_graph->base_scale;
			vertices_buff[vert_index]->y = atof(strtok(NULL, " ")) * scene_graph->base_scale;
			vertices_buff[vert_index]->z = atof(strtok(NULL, " ")) * scene_graph->base_scale;
			vert_index++;
		}
		if (strcmp(token, "vt") == 0) {
			uvs_buff[uv_index] = (XUVCoordinate*)malloc(sizeof(XUVCoordinate));
			uvs_buff[uv_index]->u = atof(strtok(NULL, " "));
			uvs_buff[uv_index]->v = atof(strtok(NULL, " "));
			uv_index++;
		}
		if (strcmp(token, "usemtl") == 0) {
			token = strtok(NULL, "\n");
			material_names[mtl_index] = (char*)malloc((strlen(token) + 1) * sizeof(char));
			strcpy(material_names[mtl_index], token);
			material_to_object_indexes[mtl_index] = obj_index - 1;
			mtl_index++;
		}
		if (strcmp(token, "f") == 0) {
			int i = 0;
			char *ind_token;
			while ((token = strtok(NULL, " ")) != NULL) {
				ind_token = token;
				token = strtok_r(ind_token, "/", &ind_token);
				face_indexes[i].vertex = atoi(token);
				token = strtok_r(ind_token, " ", &ind_token);
				face_indexes[i].uv = atoi(token);
				i++;
			}
			faces_num_indexes[face_index] = i;
			faces_buff[face_index] = (FaceIndex*)malloc(i * sizeof(FaceIndex));
			memcpy(faces_buff[face_index], face_indexes, i * sizeof(FaceIndex));
			face_index++;
		}
	}
	object_start_vertices[obj_index] = vert_index;
	object_start_uvs[obj_index] = uv_index;
	object_start_faces[obj_index] = face_index;

	/* create XSceneGraph */

	/* create XSceneGraph */
	scene_graph->poly_graphs = (XPolyGraph**)malloc((obj_index + 1) * sizeof(XPolyGraph*));
	for (int i = 0; i < obj_index; i++) {
		scene_graph->poly_graphs[i] = (XPolyGraph*)malloc(sizeof(XPolyGraph));
		scene_graph->poly_graphs[i]->name = (char*)malloc((strlen(object_names[i]) + 1) * sizeof(char));
		scene_graph->poly_graphs[i]->material_index = -1;
		strcpy(scene_graph->poly_graphs[i]->name, object_names[i]);
		scene_graph->poly_graphs[i + 1] = NULL;
	}
	
	/* link materials */
	int m_i;
	for (int k = 0; k < mtl_index; k++) {
		m_i = 0;
		while (scene_graph->materials[m_i] != NULL) {
			if (strcmp(scene_graph->materials[m_i]->name, material_names[k]) == 0) {
				scene_graph->poly_graphs[material_to_object_indexes[k]]->material_index = m_i;
			}
			m_i++;
		}
	}

	/* copy vertices */
	for (int k = 0; k < obj_index; k++) {
		graph = scene_graph->poly_graphs[k];
		graph->vertices = (XVertex**)malloc((vert_index + 1) * sizeof(XVertex*));
		int pos = 0;
		for (int i = object_start_vertices[k]; i < object_start_vertices[k + 1]; i++) {
			graph->vertices[pos] = vertices_buff[i];
			graph->vertices[pos + 1] = NULL;
			pos++;
		}
	}
	
	/* copy uv coordinates */
	for (int k = 0; k < obj_index; k++) {
		graph = scene_graph->poly_graphs[k];
		graph->uvs = (XUVCoordinate**)malloc((uv_index + 1) * sizeof(XUVCoordinate*));
		int pos = 0;
		for (int i = object_start_uvs[k]; i < object_start_uvs[k + 1]; i++) {
			graph->uvs[pos] = uvs_buff[i];
			graph->uvs[pos + 1] = NULL;
			pos++;
		}
	}

	/* create faces */
	for (int k = 0; k < obj_index; k++) {
		graph = scene_graph->poly_graphs[k];
		graph->faces = (XFace**)malloc((face_index + 1) * sizeof(XFace*));
		int pos = 0;
		for (int i = object_start_faces[k]; i < object_start_faces[k + 1]; i++) {
			graph->faces[pos] = (XFace*)malloc(sizeof(XFace));
			graph->faces[pos]->vertices = (XVertex**)malloc((faces_num_indexes[i] + 1) * sizeof(XVertex*));
			graph->faces[pos]->uvs = (XUVCoordinate**)malloc((faces_num_indexes[i] + 1) * sizeof(XUVCoordinate*));
			graph->faces[pos]->num_verts = faces_num_indexes[i];
			for (int j = 0; j < faces_num_indexes[i]; j++) {
				/* copy faces */
				graph->faces[pos]->vertices[j] = vertices_buff[faces_buff[i][j].vertex - 1];
				graph->faces[pos]->uvs[j] = uvs_buff[faces_buff[i][j].uv - 1];
				graph->faces[pos]->vertices[j + 1] = NULL;
				graph->faces[pos]->uvs[j + 1] = NULL;
			}
			free(faces_buff[i]);
			graph->faces[pos + 1] = NULL;
			pos++;
		}
	}

	#ifdef DEBUG
		fprintf(stderr, "Number of objects: %d\n", obj_index);
		fprintf(stderr, "Number of vertices: %d\n", vert_index);
		fprintf(stderr, "Number of uvs: %d\n", uv_index);
		fprintf(stderr, "Number of faces: %d\n", face_index);
	#endif

	return 1;
}

int XLoadMTLLib(FILE *file, XSceneGraph *scene_graph) {
	char buff[STR_BUFF_SIZE];
	char *token;
	
	XMaterial *materials_buff[BUFF_SIZE];
	int mtl_index = 0;

	while ( 1 ) {
		fgets(buff, STR_BUFF_SIZE, file);
		if (feof(file))
			break;

		token = strtok(buff, " ");
		if (strcmp(token, "newmtl") == 0) {
			materials_buff[mtl_index] = (XMaterial*)malloc(sizeof(XMaterial));
			
			token = strtok(NULL, "\n");
			materials_buff[mtl_index]->name = (char*)malloc((strlen(token) + 1) * sizeof(char));
			strcpy(materials_buff[mtl_index]->name, token);
			
			materials_buff[mtl_index]->diffuse_tex_filename = NULL;
			materials_buff[mtl_index]->diffuse_tex_id = 0;
			
			mtl_index++;
		}
		if (strcmp(token, "map_Kd") == 0) {
			token = strtok(NULL, "\n");
			materials_buff[mtl_index - 1]->diffuse_tex_filename = (char*)malloc((strlen(token) + 1) * sizeof(char));
			strcpy(materials_buff[mtl_index - 1]->diffuse_tex_filename, token);
		}
	}
	
	/* copy materials to scene */
	scene_graph->materials = (XMaterial**)malloc((mtl_index + 1) * sizeof(XMaterial*));
	for (int i = 0; i < mtl_index; i++) {
		scene_graph->materials[i] = materials_buff[i];
		scene_graph->materials[i + 1] = NULL;
	}
	
	#ifdef DEBUG
		fprintf(stderr, "Number of materials: %d\n", mtl_index);
	#endif
	
	return 1;
}

int XLoadTextures(XSceneGraph *scene_graph, char *path_mtl) {
	XMaterial **m_p = scene_graph->materials, *material;
	while (*m_p != NULL) {
		material = *m_p;
		if (material->diffuse_tex_filename != NULL) {
			material->diffuse_tex_id = loadGlTextureFromFile(material->diffuse_tex_filename, path_mtl);
		}
		m_p++;
	}
	return 1;
}

void XFreeSceneGraph(XSceneGraph *scene_graph) {
	XVertex **v_p;
	XUVCoordinate **uv_p;
	XFace **f_p;
	XPolyGraph *graph, **g_p;
	XMaterial *material, **mtl_p;

	g_p = scene_graph->poly_graphs;
	while (*g_p != NULL) {
		graph = *g_p;
		free(graph->name);

		v_p = graph->vertices;
		while (*v_p != NULL) {
			free(*v_p);
			v_p++;
		}
		free(graph->vertices);
		
		uv_p = graph->uvs;
		while (*uv_p != NULL) {
			free(*uv_p);
			uv_p++;
		}
		free(graph->uvs);
		

		f_p = graph->faces;
		while (*f_p != NULL) {
			free((*f_p)->vertices);
			free((*f_p)->uvs);

			free(*f_p);
			f_p++;
		}
		free(graph->faces);
		g_p++;
	}
	free(scene_graph->poly_graphs);
	
	mtl_p = scene_graph->materials;
	while (*mtl_p != NULL) {
		material = *mtl_p;
		
		free(material->name);
		free(material->diffuse_tex_filename);
		
		mtl_p++;
	}
	free(scene_graph->materials);
}


void XDrawSceneGraph(XSceneGraph *scene_graph) {
	XVertex *start_vertex;
	XFace *face, **f_p;
	XPolyGraph *graph, **g_p;
	XMaterial *material;
	
	g_p = scene_graph->poly_graphs;
	while (*g_p != NULL) {
		graph = *g_p;
		
		if (graph->material_index >= 0) {
			material = scene_graph->materials[graph->material_index];
			if (material->diffuse_tex_id > 0) {
				//printf("tex_id: %d\n", material->diffuse_tex_id);
				glBindTexture(GL_TEXTURE_2D, material->diffuse_tex_id);
			}
		}
		
		f_p = graph->faces;
		while (*f_p != NULL) {
			face = *f_p;
			
			glBegin(GL_POLYGON);
			if (face->num_verts >= 3) {
				for (int i = 0; i < face->num_verts; i++) {
					//printf("%lf %lf\n", face->uvs[i]->u, face->uvs[i]->v);
					glTexCoord2f(face->uvs[i]->u, 1.0f - face->uvs[i]->v);
					glVertex3f(face->vertices[i]->x, face->vertices[i]->y, face->vertices[i]->z);
				}
			}
			
			glEnd();
			
			f_p++;
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);
		
		g_p++;
	}
}

