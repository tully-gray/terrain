#include "maths.h"


void updateFogLights(GLfloat *clear, GLfloat *ambient, float camheight, int squaresize, float *fogend)
{
  static float fogstart = 10.0f;
  float temp = 0.0f;
  float lightspec[4];
  float lightamb[4];
  float lightdiff[4];

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
  glClearColor(clear[0], clear[1], clear[2], clear[3]);
  temp = squaresize * TERRAIN_GRID_SIZE * 0.9f;
  if (*fogend > temp)
    *fogend -= (*fogend - temp) * 0.1f;
  else
    *fogend -= (*fogend - temp) * 0.04f;
  temp = *fogend * 0.35f;
  if (fogstart > temp)
    fogstart -= (fogstart - temp) * 0.1f;
  else
    fogstart -= (fogstart - temp) * 0.02f;
  fogstart = fogstart > 37000 ? 37000 : fogstart;
  glFogfv(GL_FOG_COLOR, clear);
  glFogf(GL_FOG_START, fogstart);
  glFogf(GL_FOG_END, *fogend);
  lightspec[0] = 0.12f;
  lightspec[1] = 0.1f;
  lightspec[2] = 0.11f;
  glLightfv(GL_LIGHT1, GL_SPECULAR, lightspec);
  lightamb[0] = 0.0f;
  lightamb[1] = 0.0f;
  lightamb[2] = 0.0f;
  glLightfv(GL_LIGHT1, GL_AMBIENT, lightamb);
  lightdiff[0] = 0.25f + sinf(camheight*0.00057f) * 0.15f;
  lightdiff[1] = 0.17f;
  lightdiff[2] = 0.19f;
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightdiff);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-5.4f, 5.4f, -4.0f, 4.0f, 8.0f, *fogend * 1.1f);
  glMatrixMode(GL_MODELVIEW);
}


void renderFoliage(struct model *models, struct v3f camerapos, struct v3f camerarot, struct v2f sector, float camheight)
{
  int xgrid, zgrid, x1, z1, cull;
  int squaresize = TERRAIN_SQUARE_SIZE * 0.8f;
  float x, z, xpos = 0.0f, zpos = 0.0f, dist;
  struct terrain temp;
  GLubyte alpha;

  glMateriali(GL_FRONT, GL_SHININESS, 37);
  x = (int) (sector.x / squaresize);
  z = (int) (sector.y / squaresize);
  for (xgrid = 0, zgrid = 0; xgrid < TERRAIN_GRID_SIZE && zgrid < TERRAIN_GRID_SIZE; xgrid++) {
    xpos = (xgrid - TERRAIN_GRID_SIZE_HALF + x) * squaresize;
    zpos = (zgrid - TERRAIN_GRID_SIZE_HALF + z) * squaresize;
    x1 = (int) xpos % 297;
    z1 = (int) zpos % 153;
    xpos += z1;
    zpos += x1;
    cull = fabs((int) (camerarot.y - vectorstodegree2d(camerapos, mv3f(xpos, 0, zpos))));
    while (cull >= 360)
      cull -= 360;
    dist = distance3d(camerapos, mv3f(xpos, -camheight, zpos));
    x1 = x1 * x1 + z1 * z1;
    if ((cull <= 85 || cull >= 275 || fabs(camerarot.x) > 27.0f) && dist < VIEW_DISTANCE && (x1 % 1176 < 137)) {
      temp = readTerrain(-xpos, -zpos);
      if (temp.height > TERRAIN_WATER_LEVEL + 50 && temp.height < 3750 && temp.type != T_TYPE_DIRT) {
        if (dist < VIEW_DISTANCE_HALF)
          alpha = 255;
        else if (dist < VIEW_DISTANCE)
          alpha = (GLubyte) (255 - ((dist - VIEW_DISTANCE_HALF) / (float) VIEW_DISTANCE_HALF) * 255);
        else
          alpha = 0;
        drawModel(models[x1 % 6], mv3f(xpos, temp.height, zpos), mv3f(0, x1, 0), 3, alpha);
        xpos += z1;
        zpos -= z1;
        temp = readTerrain(-xpos, -zpos);
        drawModel(models[(int)fabs(z1) % 6], mv3f(xpos, temp.height, zpos), mv3f(0, z1, 0), 3, alpha);
      }
    }
    if (xgrid >= TERRAIN_GRID_SIZE - 1) {
      zgrid++;
      xgrid = -1;
    }
  }
}


void renderSky(struct v3f camerapos, struct v3f camerarot, GLfloat *clear, float fogend)
{
  glPushMatrix();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glTranslatef(-camerapos.x, -camerapos.y, -camerapos.z);
  glRotatef((GLfloat) (-camerarot.y), 0.0f, 1.0f, 0.0f);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
  glColor3fv(clear);
  glVertex3f(250000.0f, 3000.0f, 250000.0f);
  glVertex3f(-250000.0f, 3000.0f, 250000.0f);
  glColor3ub(117, 132, 215);
  glVertex3f(-250000.0f, 3000.0f, -fogend);
  glVertex3f(250000.0f, 3000.0f, -fogend);
  glVertex3f(250000.0f, 3000.0f, -fogend);
  glVertex3f(-250000.0f, 3000.0f, -fogend);
  glColor3fv(clear);
  glVertex3f(-250000.0f, -5000.0f, -fogend);
  glVertex3f(250000.0f, -5000.0f, -fogend);
  glEnd();
  glPopMatrix();
}


void renderWater(struct v3f camerapos, struct v3f camerarot, int *squaresize)
{
  int xshift, zshift, xgrid, zgrid, size = *squaresize * 8;
  float xpos, zpos;

  glMateriali(GL_FRONT, GL_SHININESS, 17);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glPushMatrix();
  glTranslatef(-camerapos.x, 0.0f, -camerapos.z);
  glRotatef((GLfloat) (-camerarot.y), 0.0f, 1.0f, 0.0f);
  glBegin(GL_QUADS);
  glColor4ub(32, 112, 255, 110);
  glNormal3i(0, -1, 0);
  for (xgrid = 0, zgrid = 0; xgrid < TERRAIN_GRID_SIZE_HALF && zgrid < TERRAIN_GRID_SIZE_HALF; xgrid++) {
    xshift = zshift = size;
    xpos = (-TERRAIN_GRID_SIZE_QUARTER + xgrid) * xshift;
    zpos = (-TERRAIN_GRID_SIZE_QUARTER + zgrid) * zshift;
    xshift = zshift = size / 2;
    glVertex3f(xpos + xshift, TERRAIN_WATER_LEVEL, zpos + zshift);
    glVertex3f(xpos - xshift, TERRAIN_WATER_LEVEL, zpos + zshift);
    glVertex3f(xpos - xshift, TERRAIN_WATER_LEVEL, zpos - zshift);
    glVertex3f(xpos + xshift, TERRAIN_WATER_LEVEL, zpos - zshift);
    if (xgrid >= TERRAIN_GRID_SIZE_HALF - 1) {
      zgrid++;
      xgrid = -1;
    }
  }
  glEnd();
  glPopMatrix();
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}


void renderCloud(struct v3f camerapos, struct v3f camerarot, int *squaresize)
{
  int xshift, zshift, xgrid, zgrid, size = *squaresize * 8;
  float xpos, zpos, height = 11500.0f, scale = 0.00005f;

  glMateriali(GL_FRONT, GL_SHININESS, 161);
  glDisable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glPushMatrix();
  glTranslatef(-camerapos.x, 0.0f, -camerapos.z);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(-camerapos.x*scale, 0.0f, -camerapos.z*scale);
  glScalef(scale, scale, scale);
  glBegin(GL_QUADS);
  glColor4ub(128, 128, 128, 60);
  glNormal3i(0, -1, 0);
  for (xgrid = 0, zgrid = 0; xgrid < TERRAIN_GRID_SIZE_HALF && zgrid < TERRAIN_GRID_SIZE_HALF; xgrid++) {
    xshift = zshift = size;
    xpos = (-TERRAIN_GRID_SIZE_QUARTER + xgrid) * xshift;
    zpos = (-TERRAIN_GRID_SIZE_QUARTER + zgrid) * zshift;
    xshift = zshift = size / 2;
    glTexCoord2i(xpos + xshift, zpos + zshift);
    glVertex3f(xpos + xshift, height, zpos + zshift);
    glTexCoord2i(xpos - xshift, zpos + zshift);
    glVertex3f(xpos - xshift, height, zpos + zshift);
    glTexCoord2i(xpos - xshift, zpos - zshift);
    glVertex3f(xpos - xshift, height, zpos - zshift);
    glTexCoord2i(xpos + xshift, zpos - zshift);
    glVertex3f(xpos + xshift, height, zpos - zshift);
    if (xgrid >= TERRAIN_GRID_SIZE_HALF - 1) {
      zgrid++;
      xgrid = -1;
    }
  }
  glEnd();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  //glEnable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}


void render(GLFWwindow *window, struct model *models, GLuint *textures, GLuint *shaders, int *swapb, struct v3f camerapos, struct v3f camerarot, struct v2f *sector, float camheight, int *squaresize, float *fogend, struct v3f playerpos, struct v3f playerrot)
{
  GLfloat materialColor[4];
  GLfloat clear[4]   = {0.5f, 0.5f, 0.5f, 1.0f};
  GLfloat ambient[4] = {0.49f, 0.45f, 0.47f, 1.0f};

  materialColor[3] = 1.0f;
  materialColor[0] = materialColor[1] = materialColor[2] = 0.8f;
  glMaterialfv(GL_FRONT, GL_DIFFUSE, materialColor);
  glMaterialfv(GL_FRONT, GL_AMBIENT, materialColor);
  glMaterialfv(GL_FRONT, GL_SPECULAR, materialColor);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glUseProgramARB(0);
  renderSky(camerapos, camerarot, clear, *fogend);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_FOG);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  //glUseProgramARB(shaders[0]);
  //glActiveTextureARB(GL_TEXTURE0_ARB);
  glBindTexture(GL_TEXTURE_2D, textures[0]);
  //glUniform1iARB(glGetUniformLocationARB(shaders[0], "scene"), 0);
  drawTerrain(camerapos, camerarot, sector, camheight, swapb, squaresize);
  renderWater(camerapos, camerarot, squaresize);
  clear[2] += fabs(sinf(camheight*0.00067f)) * 0.23f;
  updateFogLights(clear, ambient, camheight, *squaresize, fogend);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  //glEnableClientState(GL_TEXTURE_COORD_ARRAY); /* this does not currently work */
  glBindTexture(GL_TEXTURE_2D, textures[1]);
  renderFoliage(models, camerapos, camerarot, *sector, camheight);
  materialColor[0] = materialColor[1] = materialColor[2] = 0.1f;
  glBindTexture(GL_TEXTURE_2D, textures[3]);
  drawModel(models[6], mv3f(playerpos.x, -playerpos.y, playerpos.z), mv3f(playerrot.x, 180 - playerrot.y, playerrot.z), 20, 255);  
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glBindTexture(GL_TEXTURE_2D, textures[2]);
  renderCloud(camerapos, camerarot, squaresize);
  if (*swapb)
    glfwSwapBuffers(window);
  *swapb = 1;
  glfwPollEvents();
}