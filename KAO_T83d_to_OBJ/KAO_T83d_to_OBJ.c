////////////////////////////////////////////////////////////////
// "KAO_T83d_to_OBJ.c"
////////////////////////////////////////////////////////////////

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <errno.h>
#include <direct.h>

////////////////////////////////////////////////////////////////
// Constants definitions
////////////////////////////////////////////////////////////////

const uint8_t PI_DBL[8] = {0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40};
#define USHORT_HALFTURN  0x8000

#define BUFFER_SIZE  256

#define KAO_ANIMMESH_MAGIC  0x64333854
#define KAO_ANIMMESH_VERSION  0x0C
#define KAO_ANIMMESH_BNDBOX_SIZE  (3 * 3 * 4 + 4)

#define MTL_FILENAME  "texture.mtl"
#define DEFAULT_MATERIAL_NAME  "texture"

#define TRIANGLE_FAN    0
#define TRIANGLE_STRIP  1

////////////////////////////////////////////////////////////////
// Structure definitions
////////////////////////////////////////////////////////////////

typedef struct BinaryFile binfile_t;
struct BinaryFile
{
    FILE * file_ptr;
};

typedef struct Vertex vert_t;
struct Vertex
{
    float x;
    float y;
    float z;
};

typedef struct Angle angle_t;
struct Angle
{
    double angle;
    double sin;
    double cos;
};

typedef struct Rotation rot_t;
struct Rotation
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
};

typedef struct RenderVertex rvert_t;
struct RenderVertex
{
    uint16_t index;
    uint8_t u;
    uint8_t v;
};

typedef struct AnimKeyframe animkey_t;
struct AnimKeyframe
{
    uint8_t vert_group;
    uint8_t coord_mode;
    rot_t rot;
    vert_t pos;
};

typedef struct AnimObject animobj_t;
struct AnimObject
{
    uint32_t num_verts;
    uint32_t num_groups;
    char name[32];
    vert_t ** vertices;
};

typedef struct AnimMotion animmotion_t;
struct AnimMotion
{
    uint32_t num_keyframes;
    char name[32];
    animkey_t ** keys;
};

typedef struct AnimCommand animcommand_t;
struct AnimCommand
{
    uint32_t num_indices;
    uint8_t transparency;
    uint8_t triangle_mode;
    rvert_t * indices;
};

typedef struct AnimMesh animmesh_t;
struct AnimMesh
{
    uint32_t total_vertices;
    vert_t * vertices;

    uint32_t num_objects;
    animobj_t * objects;

    uint32_t total_keyframes;
    uint32_t num_motions;
    animmotion_t * motions;

    uint32_t num_commands;
    animcommand_t * commands;
};

////////////////////////////////////////////////////////////////
// struct BinaryFile functions
////////////////////////////////////////////////////////////////

uint32_t BinaryFile_open(binfile_t * me, const char * path)
{
    me->file_ptr = fopen(path, "rb");

    if (NULL == (me->file_ptr))
    {
        fprintf(stderr, "Could not open file: \"%s\"\n", path);
        return 1;
    }

    return 0;
}

void BinaryFile_close(binfile_t * me)
{
    fclose(me->file_ptr);
}

void BinaryFile_read(binfile_t * me, void * destination, int length)
{
    fread(destination, length, 0x01, me->file_ptr);
}

void BinaryFile_skip(binfile_t * me, int length)
{
    fseek(me->file_ptr, length, SEEK_CUR);
}

////////////////////////////////////////////////////////////////
// struct Vertex functions
////////////////////////////////////////////////////////////////

void Vertex_read(vert_t * me, binfile_t * file)
{
    BinaryFile_read(file, &(me->x), 0x04);
    BinaryFile_read(file, &(me->y), 0x04);
    BinaryFile_read(file, &(me->z), 0x04);
}

void Vertex_read_aux(vert_t * me, binfile_t * file)
{
    Vertex_read(me, file);
    BinaryFile_skip(file, 0x04);
}

////////////////////////////////////////////////////////////////
// struct Rotation functions
////////////////////////////////////////////////////////////////

void Rotation_read(rot_t * me, binfile_t * file)
{
    BinaryFile_read(file, &(me->x), 0x02);
    BinaryFile_read(file, &(me->y), 0x02);
    BinaryFile_read(file, &(me->z), 0x02);
}

void Rotation_get_angles(rot_t * me, double * x, double * y, double * z)
{
    const double pi = * (const double *) PI_DBL;

    (* x) = me->x * pi / USHORT_HALFTURN;
    (* y) = me->y * pi / USHORT_HALFTURN;
    (* z) = me->z * pi / USHORT_HALFTURN;
}

////////////////////////////////////////////////////////////////
// struct RenderVertex functions
////////////////////////////////////////////////////////////////

void RenderVertex_read(rvert_t * me, binfile_t * file)
{
    BinaryFile_read(file, &(me->index), 0x02);
    BinaryFile_read(file, &(me->u), 0x01);
    BinaryFile_read(file, &(me->v), 0x01);
}

void RenderVertex_get_mapping(rvert_t * me, double * u, double * v)
{
    /* V texture mapping is explicitly inverted */

    (*u) =   ((double) me->u) / 256.0;
    (*v) = - ((double) me->v) / 256.0;
}

////////////////////////////////////////////////////////////////
// struct AnimKeyframe functions
////////////////////////////////////////////////////////////////

void AnimKeyframe_read(animkey_t * me, binfile_t * file)
{
    BinaryFile_read(file, &(me->vert_group), 0x01);
    BinaryFile_read(file, &(me->coord_mode), 0x01);
    Rotation_read(&(me->rot), file);
    Vertex_read(&(me->pos), file);
}

////////////////////////////////////////////////////////////////
// struct AnimObject functions
////////////////////////////////////////////////////////////////

void AnimObject_init(animobj_t * me)
{
    me->vertices = NULL;
}

void AnimObject_destroy(animobj_t * me)
{
    uint32_t i;

    if (NULL != (me->vertices))
    {
        for (i = 0; i < (me->num_groups); i++)
        {
            if (NULL != (me->vertices[i]))
            {
                free(me->vertices[i]);
            }
        }

        free(me->vertices);
    }
}

uint32_t AnimObject_read_header(animobj_t * me, binfile_t * file, uint32_t * total_verts)
{
    BinaryFile_read(file, &(me->num_verts), 0x04);
    BinaryFile_read(file, &(me->num_groups), 0x04);
    BinaryFile_read(file, me->name, 32);
    BinaryFile_skip(file, 0x04);

    if ((0 != (me->num_verts)) && (0 == (me->num_groups)))
    {
        fputs("AnimObject header error: non-zero vertices and zero groups.", stderr);
        return 1;
    }
    if ((0 == (me->num_verts)) && (0 != (me->num_groups)))
    {
        fputs("AnimObject header error: zero vertices and non-zero groups.", stderr);
        return 1;
    }

    (* total_verts) += (me->num_verts);
    return 0;
}

uint32_t AnimObject_read_vgroups(animobj_t * me, binfile_t * file)
{
    uint32_t i, j;

    if (0 == (me->num_groups))
    {
        return 0;
    }

    me->vertices = (vert_t **) malloc(sizeof(vert_t *) * (me->num_groups));

    if (NULL == (me->vertices))
    {
        fputs("Memory allocation error! (AnimObject vertex groups)", stderr);
        return 1;
    }

    for (i = 0; i < (me->num_groups); i++)
    {
        me->vertices[i] = NULL;
    }

    for (i = 0; i < (me->num_groups); i++)
    {
        me->vertices[i] = (vert_t *) malloc(sizeof(vert_t) * (me->num_verts));

        if (NULL == (me->vertices[i]))
        {
            fputs("Memory allocation error! (AnimObject vertex group data)", stderr);
            return 1;
        }
    }

    for (i = 0; i < (me->num_groups); i++)
    {
        for (j = 0; j < (me->num_verts); j++)
        {
            Vertex_read_aux(&(me->vertices[i][j]), file);
        }
    }

    return 0;
}

void AnimObject_calculate_verts(animobj_t * me, animkey_t * key, vert_t ** output)
{
    uint32_t i;
    vert_t pos[2];
    angle_t rx, ry, rz;

    vert_t * final_verts = (* output);
    vert_t * group_verts;

    if (0 == (me->num_groups))
    {
        return;
    }

    if (NULL != key)
    {
        group_verts = (me->vertices[key->vert_group]);

        Rotation_get_angles(&(key->rot), &(rx.angle), &(ry.angle), &(rz.angle));

        rx.sin = sin(rx.angle);
        rx.cos = cos(rx.angle);
        
        ry.sin = sin(ry.angle);
        ry.cos = cos(ry.angle);
        
        rz.sin = sin(rz.angle);
        rz.cos = cos(rz.angle);

        for (i = 0; i < (me->num_verts); i++)
        {
            /* Euler rotation */
            
            pos[0] = (* group_verts);

            pos[1].y =   pos[0].y * rx.cos - pos[0].z * rx.sin;
            pos[1].z =   pos[0].y * rx.sin + pos[0].z * rx.cos;

            pos[0].z = pos[1].z;

            pos[1].x =   pos[0].x * ry.cos + pos[0].z * ry.sin;
            pos[1].z = - pos[0].x * ry.sin + pos[0].z * ry.cos;

            pos[0].x = pos[1].x;
            pos[0].y = pos[1].y;

            pos[1].x =   pos[0].x * rz.cos - pos[0].y * rz.sin;
            pos[1].y =   pos[0].x * rz.sin + pos[0].y * rz.cos;

            /* Translation */

            if (0x80 == (key->coord_mode))
            {
                (final_verts->x) = (key->pos.x) - pos[1].x;
                (final_verts->y) = (key->pos.y) - pos[1].y;
                (final_verts->z) = (key->pos.z) - pos[1].z;
            }
            else
            {
                (final_verts->x) = (key->pos.x) + pos[1].x;
                (final_verts->y) = (key->pos.y) + pos[1].y;
                (final_verts->z) = (key->pos.z) + pos[1].z;
            }

            group_verts++;
            final_verts++;
        }
    }
    else
    {
        group_verts = (me->vertices[0]);

        for (i = 0; i < (me->num_verts); i++)
        {
            (* final_verts) = (* group_verts);

            group_verts++;
            final_verts++;
        }
    }

    (* output) = final_verts;
}

////////////////////////////////////////////////////////////////
// struct AnimMotion functions
////////////////////////////////////////////////////////////////

void AnimMotion_init(animmotion_t * me)
{
    me->keys = NULL;
}

void AnimMotion_destroy(animmotion_t * me)
{
    uint32_t i;

    if (NULL != (me->keys))
    {
        for (i = 0; i < (me->num_keyframes); i++)
        {
            if (NULL != (me->keys[i]))
            {
                free(me->keys[i]);
            }
        }

        free(me->keys);
    }
}

uint32_t AnimMotion_read_header(animmotion_t * me, binfile_t * file, uint32_t * total_keyframes)
{
    BinaryFile_read(file, &(me->num_keyframes), 0x04);
    BinaryFile_read(file, me->name, 32);

    if (0 == (me->num_keyframes))
    {
        fputs("AnimMotion header error: zero keyframes", stderr);
        return 1;
    }

    (* total_keyframes) += (me->num_keyframes);

    return 0;
}

uint32_t AnimMotion_read_keyframes(animmotion_t * me, binfile_t * file, uint32_t num_objects)
{
    uint32_t i, j;

    me->keys = (animkey_t **) malloc(sizeof(animkey_t *) * (me->num_keyframes));

    if (NULL == (me->keys))
    {
        fputs("Memory allocation error! (AnimMotion keyframes)", stderr);
        return 1;
    }

    for (i = 0; i < (me->num_keyframes); i++)
    {
        me->keys[i] = NULL;
    }


    for (i = 0; i < (me->num_keyframes); i++)
    {
        me->keys[i] = (animkey_t *) malloc(sizeof(animkey_t) * num_objects);

        if (NULL == (me->keys[i]))
        {
            fputs("Memory allocation error! (AnimMotion keyframe objects)", stderr);
            return 1;
        }
    }

    for (i = 0; i < (me->num_keyframes); i++)
    {
        for (j = 0; j < num_objects; j++)
        {
            AnimKeyframe_read(&(me->keys[i][j]), file);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////
// struct AnimCommand functions
////////////////////////////////////////////////////////////////

void AnimCommand_init(animcommand_t * me)
{
    me->indices = NULL;
}

void AnimCommand_destroy(animcommand_t * me)
{
    if (NULL != (me->indices))
    {
        free(me->indices);
    }
}

uint32_t AnimCommand_read(animcommand_t * me, binfile_t * file)
{
    uint32_t i;
    int8_t dummy;

    BinaryFile_read(file, &dummy, 0x01);
    BinaryFile_read(file, &(me->transparency), 0x01);

    if (dummy < 0)
    {
        me->num_indices = (- dummy);
        me->triangle_mode = TRIANGLE_STRIP;
    }
    else
    {
        me->num_indices = dummy;
        me->triangle_mode = TRIANGLE_FAN;
    }

    (me->indices) = (rvert_t *) malloc(sizeof(rvert_t) * (me->num_indices));

    if (NULL == (me->indices))
    {
        fputs("Memory allocation error! (AnimCommand indices)", stderr);
        return 1;
    }

    for (i = 0; i < (me->num_indices); i++)
    {
        RenderVertex_read(&(me->indices[i]), file);
    }

    return 0;
}

////////////////////////////////////////////////////////////////
// struct AnimMesh functions
////////////////////////////////////////////////////////////////

void AnimMesh_init(animmesh_t * me)
{
    me->vertices = NULL;
    me->objects  = NULL;
    me->motions  = NULL;
    me->commands = NULL;
}

void AnimMesh_destroy(animmesh_t * me)
{
    uint32_t i;

    if (NULL != (me->vertices))
    {
        free(me->vertices);
    }

    if (NULL != (me->objects))
    {
        for (i = 0; i < (me->num_objects); i++)
        {
            AnimObject_destroy(&(me->objects[i]));
        }

        free(me->objects);
    }

    if (NULL != (me->motions))
    {
        for (i = 0; i < (me->num_motions); i++)
        {
            AnimMotion_destroy(&(me->motions[i]));
        }

        free(me->motions);
    }

    if (NULL != (me->commands))
    {
        for (i = 0; i < (me->num_commands); i++)
        {
            AnimCommand_destroy(&(me->commands[i]));
        }

        free(me->commands);
    }
}

uint32_t AnimMesh_read(animmesh_t * me, binfile_t * file)
{
    uint32_t i, num_faces;

    /* Header */

    BinaryFile_read(file, &i, 0x04);

    if (KAO_ANIMMESH_MAGIC != i)
    {
        fputs("Invalid model header! Expected \"T83d\".", stderr);
        return 1;
    }

    BinaryFile_read(file, &i, 0x04);

    if (KAO_ANIMMESH_VERSION != i)
    {
        fprintf(stderr, "Invalid model version! Expected %d", KAO_ANIMMESH_VERSION);
        return 1;
    }

    BinaryFile_skip(file, 0x04);

    BinaryFile_read(file, &(me->num_objects), 0x04);

    if ((me->num_objects) > 64)
    {
        fputs("Too many objects in model! Expected no more than 64.", stderr);
        return 1;
    }
    else if (0 == (me->num_objects))
    {
        fputs("No vertex groups in model!", stderr);
        return 1;
    }

    BinaryFile_read(file, &num_faces, 0x04);

    BinaryFile_read(file, &(me->num_motions), 0x04);

    BinaryFile_read(file, &i, 0x04);
    me->num_commands = i;
    BinaryFile_read(file, &i, 0x04);
    me->num_commands += i;

    if (0 == (me->num_commands))
    {
        fputs("Model has zero render commands!", stderr);
        return 1;
    }

    BinaryFile_skip(file, 0x04 + KAO_ANIMMESH_BNDBOX_SIZE);

    /* Headers foreach 3D object */

    me->objects = (animobj_t *) malloc (sizeof(animobj_t) * (me->num_objects));

    if (NULL == (me->objects))
    {
        fputs("Memory allocation error! (AnimMesh objects)", stderr);
        return 1;
    }

    for (i = 0; i < (me->num_objects); i++)
    {
        AnimObject_init(&(me->objects[i]));
    }

    me->total_vertices = 0;

    for (i = 0; i < (me->num_objects); i++)
    {
        if (0 != AnimObject_read_header(&(me->objects[i]), file, &(me->total_vertices)))
        {
            return 1;
        }
    }

    if (0 == (me->total_vertices))
    {
        fputs("Model has zero vertices!", stderr);
        return 1;
    }

    me->vertices = (vert_t *) malloc(sizeof(vert_t) * (me->total_vertices));

    if (NULL == (me->vertices))
    {
        fputs("Memory allocation error! (AnimMesh vertices)", stderr);
        return 1;
    }

    /* Skipping untextured faces data */

    BinaryFile_skip(file, num_faces * 0x10);

    /* Vertex data foreach 3D object */

    for (i = 0; i < (me->num_objects); i++)
    {
        if (0 != AnimObject_read_vgroups(&(me->objects[i]), file))
        {
            return 1;
        }
    }

    me->total_keyframes = 0;

    if (0 != (me->num_motions))
    {
        /* Headers foreach motion */

        me->motions = (animmotion_t *) malloc (sizeof(animmotion_t) * (me->num_motions));

        if (NULL == (me->motions))
        {
            fputs("Memory allocation error! (AnimMesh motions)", stderr);
            return 1;
        }

        for (i = 0; i < (me->num_motions); i++)
        {
            AnimMotion_init(&(me->motions[i]));
        }

        for (i = 0; i < (me->num_motions); i++)
        {
            if (0 != AnimMotion_read_header(&(me->motions[i]), file, &(me->total_keyframes)))
            {
                return 1;
            }
        }

        /* Keyframe data foreach motion */

        for (i = 0; i < (me->num_motions); i++)
        {
            if (0 != AnimMotion_read_keyframes(&(me->motions[i]), file, (me->num_objects)))
            {
                return 1;
            }
        }

        /* Skipping model offset data foreach keyframe */

        BinaryFile_skip(file, (me->total_keyframes) * (0x0C + KAO_ANIMMESH_BNDBOX_SIZE));
    }

    /* Render commands */

    me->commands = (animcommand_t *) malloc (sizeof(animcommand_t) * (me->num_commands));

    if (NULL == (me->commands))
    {
        fputs("Memory allocation error! (AnimMesh commands)", stderr);
        return 1;
    }

    for (i = 0; i < (me->num_commands); i++)
    {
        AnimCommand_init(&(me->commands[i]));
    }

    for (i = 0; i < (me->num_commands); i++)
    {
        if (0 != AnimCommand_read(&(me->commands[i]), file))
        {
            return 1;
        }
    }

    /* Tail */

    BinaryFile_read(file, &i, 0x04);

    if (KAO_ANIMMESH_MAGIC != i)
    {
        fputs("Invalid model tail! Expected \"T83d\".", stderr);
        return 1;
    }

    return 0;
}

void AnimMesh_calculate_verts(animmesh_t * me, uint32_t motion_id, uint32_t key_id)
{
    uint32_t i;
    animkey_t * keys_ptr = me->motions[motion_id].keys[key_id];
    vert_t * verts_ptr = (me->vertices);

    for (i = 0; i < (me->num_objects); i++)
    {
        AnimObject_calculate_verts(&(me->objects[i]), keys_ptr, &(verts_ptr));
        keys_ptr++;
    }
}

void AnimMesh_save_obj_model(animmesh_t * me, FILE * file, const char * rel_mtl_dir)
{
    uint32_t i, j, num_vt, idx[3][2];
    double uv[2];

    fprintf(file, "\n");
    fprintf(file, "mtllib %s%s\n", rel_mtl_dir, MTL_FILENAME);
    fprintf(file, "usemtl %s\n", DEFAULT_MATERIAL_NAME);
    fprintf(file, "\n");

    for (i = 0; i < (me->total_vertices); i++)
    {
        /* Rotate 90 degrees CW on X-axis */

        fprintf(file, "v %f %f %f\n",
            (me->vertices[i].x), (me->vertices[i].z), (- (me->vertices[i].y)));
    }

    fprintf(file, "\n");

    for (i = 0; i < (me->num_commands); i++)
    {
        for (j = 0; j < (me->commands[i].num_indices); j++)
        {
            RenderVertex_get_mapping(&(me->commands[i].indices[j]), &(uv[0]), &(uv[1]));

            fprintf(file, "vt %f %f\n",
                uv[0], uv[1]);
        }
    }

    fprintf(file, "\n");

    num_vt = 1;

    for (i = 0; i < (me->num_commands); i++)
    {
        if (TRIANGLE_STRIP == (me->commands[i].triangle_mode))
        {
            for (j = 0; j < (me->commands[i].num_indices) - 2; j++)
            {
                idx[0][0] = 1 + (me->commands[i].indices[j + 0].index);
                idx[0][1] = num_vt + j + 0;
                idx[1][0] = 1 + (me->commands[i].indices[j + 1].index);
                idx[1][1] = num_vt + j + 1;
                idx[2][0] = 1 + (me->commands[i].indices[j + 2].index);
                idx[2][1] = num_vt + j + 2;

                if (0 == (j & 0x01))
                {
                    fprintf(file, "f %d/%d %d/%d %d/%d\n",
                        idx[0][0], idx[0][1], idx[1][0], idx[1][1], idx[2][0], idx[2][1]);
                }
                else
                {
                    fprintf(file, "f %d/%d %d/%d %d/%d\n",
                        idx[0][0], idx[0][1], idx[2][0], idx[2][1], idx[1][0], idx[1][1]);
                }
            }
        }
        else
        {
            fprintf(file, "f");

            for (j = 0; j < (me->commands[i].num_indices); j++)
            {
                idx[0][0] = 1 + (me->commands[i].indices[j].index);
                idx[0][1] = num_vt + j;

                fprintf(file, " %d/%d",
                    idx[0][0], idx[0][1]);
            }

            fprintf(file, "\n");
        }

        num_vt += (me->commands[i].num_indices);
    }
}

////////////////////////////////////////////////////////////////
// Converter functions
////////////////////////////////////////////////////////////////

uint32_t main_prepare_directories(char * path)
{
    uint32_t i;
    char slash;

    for (i = 0; (path[i]) && (i < BUFFER_SIZE); i++)
    {
        switch (path[i])
        {
            case '/':
            case '\\':
            {
                slash = path[i];
                path[i] = '\0';

                if ((-1) == _mkdir(path))
                {
                    if (EEXIST != errno)
                    {
                        fprintf(stderr, "Could not create directory: \"%s\"\n", path);
                        return 1;
                    }
                }

                path[i] = slash;
            }
        }
    }

    return 0;
}

uint32_t main_store_material_file(const char * output_dir, const char * texture_path)
{
    FILE * text_file;
    char file_path[BUFFER_SIZE];

    puts("\nStoring default material info...");

    sprintf(file_path, "%s%s", output_dir, MTL_FILENAME);

    if (NULL == (text_file = fopen(file_path, "w")))
    {
        fprintf(stderr, "Could not create file: \"%s\"\n", file_path);
        return 1;
    }

    fprintf(text_file, "newmtl %s\n", DEFAULT_MATERIAL_NAME);
    fprintf(text_file, "map_Kd %s\n", texture_path);

    fclose(text_file);

    printf("MTL file (\"%s\") stored.\n", file_path);

    return 0;
}

void main_calculate_and_export(animmesh_t * mesh, const char * output_dir)
{
    uint32_t i, j, k = 0;
    FILE * text_file;
    char motion_dir[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    vert_t * verts_ptr;

    puts("Animated Model loaded:");
    printf("    %3d total vertices\n", mesh->total_vertices);
    printf("    %3d vertex groups\n", mesh->num_objects);
    printf("    %3d motions\n", mesh->num_motions);
    printf("    %3d total keyframes\n", mesh->total_keyframes);
    printf("    %3d render commands\n", mesh->num_commands);

    if (0 == (mesh->num_motions))
    {
        printf("\nModel has no motions! Storing vertices in default pose.\n");

        sprintf(file_path, "%s%03d.obj", output_dir, 0);

        printf("    %3d/%3d\r", 1, 1);

        if (NULL == (text_file = fopen(file_path, "w")))
        {
            puts("");
            fprintf(stderr, "Could not create file: \"%s\"", file_path);
            return;
        }

        verts_ptr = (mesh->vertices);

        for (i = 0; i < (mesh->num_objects); i++)
        {
            AnimObject_calculate_verts(&(mesh->objects[i]), NULL, &(verts_ptr));
        }

        AnimMesh_save_obj_model(mesh, text_file, "");

        fclose(text_file);
        k++;

        puts("");
    }
    else
    {
        for (i = 0; i < (mesh->num_motions); i++)
        {
            printf("\nSaving motion \"%s\" (%d/%d)\n",
                (mesh->motions[i].name), (1 + i), (mesh->num_motions));

            sprintf(motion_dir, "%s%02d %s\\",
                output_dir, i, (mesh->motions[i].name));

            if ((-1) == _mkdir(motion_dir))
            {
                if (EEXIST != errno)
                {
                    fprintf(stderr, "Could not create directory: \"%s\"\n", motion_dir);
                    return;
                }
            }

            for (j = 0; j < (mesh->motions[i].num_keyframes); j++)
            {
                sprintf(file_path, "%s%03d.obj", motion_dir, j);

                printf("    %3d/%3d\r", (1 + j), (mesh->motions[i].num_keyframes));

                if (NULL == (text_file = fopen(file_path, "w")))
                {
                    puts("");
                    fprintf(stderr, "Could not create file: \"%s\"", file_path);
                    return;
                }

                AnimMesh_calculate_verts(mesh, i, j);
                AnimMesh_save_obj_model(mesh, text_file, "../");

                fclose(text_file);
                k++;
            }

            puts("");
        }
    }

    /* Animmesh stats */

    printf("\nStored %d keyframes (OBJ files) in total.\n\n", k);

    for (i = 0; i < (mesh->num_motions); i++)
    {
        printf("    (\"%02d %s\", %d)\n",
            i, (mesh->motions[i].name), (mesh->motions[i].num_keyframes));
    }

    puts("");
}

int main()
{
    uint32_t i;
    char model_path[BUFFER_SIZE];
    char texture_path[BUFFER_SIZE];
    char output_dir[BUFFER_SIZE];

    binfile_t model_file;
    animmesh_t mesh;

    /* User input */

    printf("\n[KAO T83d to OBJ]\n(\"Kao the Kangaroo\" animated keyframes exporter)\n\n");

    printf("Enter model (\"output.bin\") path: ");
    if (NULL == fgets(model_path, BUFFER_SIZE, stdin))
    {
        return 1;
    }
    model_path[strlen(model_path) - 1] = '\0';

    printf("Enter texture (\"texture.bmp\") path: ");
    if (NULL == fgets(texture_path, BUFFER_SIZE, stdin))
    {
        return 1;
    }
    texture_path[strlen(texture_path) - 1] = '\0';

    printf("Enter output directory path: ");
    if (NULL == fgets(output_dir, BUFFER_SIZE, stdin))
    {
        return 1;
    }
    output_dir[strlen(output_dir) - 1] = '\\';

    /* Try to open given model file */

    if (0 != BinaryFile_open(&model_file, model_path))
    {
        return 1;
    }

    /* Make sure that output path exists */

    if (0 != main_prepare_directories(output_dir))
    {
        BinaryFile_close(&model_file);
        return 1;
    }

    /* Create output MTL */

    if (0 != main_store_material_file(output_dir, texture_path))
    {
        BinaryFile_close(&model_file);
        return 1;
    }

    /* Load input model and store separate keyframes */

    AnimMesh_init(&mesh);

    puts("\nReading AnimMesh...");
    i = AnimMesh_read(&mesh, &model_file);

    BinaryFile_close(&model_file);

    if (0 == i)
    {
        main_calculate_and_export(&mesh, output_dir);
    }

    AnimMesh_destroy(&mesh);

    puts("Bye!");

    return 0;
}
