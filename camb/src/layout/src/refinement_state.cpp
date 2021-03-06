/****************************************************************************
 * Copyright (C) 2009-2012 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "layout/refinement_state.h"

using namespace indigo;

IMPL_ERROR(RefinementState, "refinement");

RefinementState::RefinementState (MoleculeLayoutGraph &graph) :
dist(0.f),
energy(0),
height(0.f),
TL_CP_GET(layout),
_graph(graph)
{
}

void RefinementState::calcDistance (int v1, int v2)
{
   Vec2f d;

   d.diff(layout[v1], layout[v2]);

   dist = d.lengthSqr();
}

void RefinementState::calcHeight ()
{
   float min = 1000.f, max = -1000.f;
   int i;

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      if (layout[i].y < min)
         min = layout[i].y;
      if (layout[i].y > max)
         max = layout[i].y;
   }

   height = max - min;
}

void RefinementState::copy (const RefinementState &other)
{
   dist = other.dist;
   energy = other.energy;
   height = other.height;

   layout.copy(other.layout);
}

void RefinementState::copyFromGraph ()
{
   int i;

   layout.clear_resize(_graph.vertexEnd());

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      layout[i] = _graph.getPos(i);
}

void RefinementState::applyToGraph ()
{
   int i;

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      _graph.getPos(i) = layout[i];
}

void RefinementState::calcEnergy ()
{
   int i,  j;
   float r;
   Vec2f d;

   energy = 0;

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      for (j = _graph.vertexBegin(); j < _graph.vertexEnd(); j = _graph.vertexNext(j))
      {
         if (i == j)
            continue;

         d.diff(layout[i], layout[j]);
         r = d.lengthSqr();

         if (r < 0.0001f)
            r = 5000000.f;
         else
            r = 1 / r;

         energy += r;
      }

   energy /= 2;
}

// Flip all verices from branch around (v1,v2)
void RefinementState::flipBranch (const Filter &branch, const RefinementState &state, int v1_idx, int v2_idx)
{
   int i;
   float r,  t;

   const Vec2f &v1 = state.layout[v1_idx];
   const Vec2f &v2 = state.layout[v2_idx];
   Vec2f d;

   d.diff(v2, v1);

   r = d.lengthSqr();

   if (r < 0.000000001f)
      throw Error("too small edge");

   layout.clear_resize(state.layout.size());

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      if  (!branch.valid(i)) 
      {
         const Vec2f &vi = state.layout[i];

         t = ((vi.x - v1.x) * d.x + (vi.y - v1.y) * d.y) / r;
         layout[i].set(2 * d.x * t + 2 * v1.x - vi.x, 2 * d.y * t + 2 * v1.y - vi.y);
      } else
         layout[i] = state.layout[i];
   }
}

// Rotate branch around vertex v1
void RefinementState::rotateBranch (const Filter &branch, const RefinementState &state, int v_idx, float angle) 
{
   int i;
   float co,  si;

   const Vec2f &v = state.layout[v_idx];
   Vec2f d;

   angle = DEG2RAD(angle);

   co = cos(angle);
   si = sin(angle);

   layout.clear_resize(state.layout.size());

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      if (!branch.valid(i)) 
      {
         d.diff(state.layout[i], v);
         d.rotate(si, co);

         layout[i].sum(d, v);
      } else
         layout[i] = state.layout[i];
   }
}

// Translate branch on 0.1 of vector [v1,v2]
void RefinementState::stretchBranch (const Filter &branch, const RefinementState &state, int v1_idx, int v2_idx, int val)
{
   int i;
   float r, sh = 0.1f * val;

   const Vec2f &v1 = state.layout[v1_idx];
   const Vec2f &v2 = state.layout[v2_idx];
   Vec2f d;

   d.diff(v2, v1);
   r = d.length();

   if (r < EPSILON) 
      throw Error("too small edge");

   d.scale(sh / r);

   if (branch.valid(v1_idx))
      d.negate();

   layout.clear_resize(state.layout.size());

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      if (!branch.valid(i)) 
         layout[i].sum(state.layout[i], d);
      else
         layout[i] = state.layout[i];
   }
}

// Rotate layout around vertex v (in degrees)
void RefinementState::rotateLayout (const RefinementState &state, int v_idx, float angle) 
{
   int i;
   float co,  si;

   const Vec2f &v = state.layout[v_idx];
   Vec2f d;

   angle = DEG2RAD(angle);

   co = cos(angle);
   si = sin(angle);

   layout.clear_resize(state.layout.size());

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      d.diff(state.layout[i], v);
      d.rotate(si, co);

      layout[i].sum(d, v);
   }
}

