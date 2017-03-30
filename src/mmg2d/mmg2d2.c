/* =============================================================================
**  This file is part of the mmg software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Bx INP/Inria/UBordeaux/UPMC, 2004- .
**
**  mmg is free software: you can redistribute it and/or modify it
**  under the terms of the GNU Lesser General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  mmg is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License and of the GNU General Public License along with mmg (in
**  files COPYING.LESSER and COPYING). If not, see
**  <http://www.gnu.org/licenses/>. Please read their terms carefully and
**  use this copy of the mmg distribution only if you accept them.
** =============================================================================
*/
/**
 * \file mmg2d/mmg2d2.c
 * \brief Mesh generation functions.
 * \author Charles Dapogny (UPMC)
 * \author Cécile Dobrzynski (Bx INP/Inria/UBordeaux)
 * \author Pascal Frey (UPMC)
 * \author Algiane Froehly (Inria/UBordeaux)
 * \version 5
 * \copyright GNU Lesser General Public License.
 */
#include "mmg2d.h"

/**
 * \param mesh pointer toward the mesh structure.
 * \return 1 if success, 0 if fail.
 *
 * Remove the bounding box triangles.
 *
 */
int MMG2_removeBBtriangles(MMG5_pMesh mesh) {
  MMG5_pTria      pt;
  int             ip1,ip2,ip3,ip4,k,iadr,*adja,iadr2,*adja2,iel,nd;
  char            i,ii;

  /* Bounding Box vertices */
  ip1 = mesh->np-3;
  ip2 = mesh->np-2;
  ip3 = mesh->np-1;
  ip4 = mesh->np;

  nd = 0;
  for(k=1; k<=mesh->nt; k++) {
    pt  = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;

    if ( pt->base < 0 ) {
      iadr = 3*(k-1) + 1;
      adja = &mesh->adja[iadr];
      for(i=0; i<3; i++) {
        if ( !adja[i] ) continue;
        iel = adja[i] / 3;
        ii = adja[i] % 3;
        iadr2 = 3*(iel-1) + 1;
        adja2 = &mesh->adja[iadr2];
        adja2 [ii] = 0;
      }
      _MMG2D_delElt(mesh,k);
      continue;
    }
    else if ( !pt->base ) {
      printf("  ## Warning: undetermined triangle %d : %d %d %d\n",
             k,pt->v[0],pt->v[1],pt->v[2]);
      nd++;
    }
  }

  if ( !nd ) {
    _MMG2D_delPt(mesh,ip1);
    _MMG2D_delPt(mesh,ip2);
    _MMG2D_delPt(mesh,ip3);
    _MMG2D_delPt(mesh,ip4);
  }
  else {
    fprintf(stdout,"  ## Error: procedure failed : %d indetermined triangles\n",nd);
    return(0);
  }
  return(1);
}

/* Set tag to triangles in the case where there are no constrained edge
   in the supplied mesh: in = base ; out = -base ; undetermined = 0*/
int MMG2_settagtriangles(MMG5_pMesh mesh,MMG5_pSol sol) {
  MMG5_pTria        pt;
  int               base,nd,iter,maxiter,k;
  int               ip1,ip2,ip3,ip4;

  /*BB vertex*/
  ip1=(mesh->np-3);
  ip2=(mesh->np-2);
  ip3=(mesh->np-1);
  ip4=(mesh->np);

  base = ++mesh->base;
  iter    = 0;
  maxiter = 3;
  do {
    nd = 0;
    for(k=1; k<=mesh->nt; k++) {
      pt = &mesh->tria[k];
      if ( !M_EOK(pt) )  continue;
      if ( !MMG2_findtrianglestate(mesh,k,ip1,ip2,ip3,ip4,base) ) nd++ ;
    }

    if(mesh->info.ddebug) printf(" ** how many undetermined triangles ? %d\n",nd);
  }
  while (nd && ++iter<maxiter);

  return(1);
}

/* Find out whether triangle pt is inside or outside (i.e. contains bb points or not) */
/* Return <0 value if triangle outside ; > 0 if triangle inside */
int MMG2_findtrianglestate(MMG5_pMesh mesh,int k,int ip1,int ip2,int ip3,int ip4,int base) {
  MMG5_pTria       pt,pt1;
  int              l,nb;
  char             i;

  pt = &mesh->tria[k];

  /* Count how many vertices of pt are vertices of the boundary box */
  nb = 0;
  for(i=0; i<3; i++)
    if ( pt->v[i] == ip1 || pt->v[i] == ip2 || pt->v[i] == ip3 || pt->v[i] == ip4 ) nb++;

  /* Triangle to be deleted */
  if ( nb ) {
    pt->base = -base;
    pt->ref = 3;
    return(-base);
  }
  else {
    //#warning check if it is true with 1 bdry edge
    pt->base = base;
    return(base);
  }
}

/**
 * \param mesh pointer toward the mesh structure
 * \param sol pointer toward the solution structure
 * \return  0 if fail.
 *
 * Insertion of the list of points inside the mesh
 * (Vertices mesh->np - 3, 2, 1, 0 are the vertices of the BB and have already been inserted)
 *
 */
int MMG2_insertpointdelone(MMG5_pMesh mesh,MMG5_pSol sol) {
  MMG5_pPoint  ppt;
  int     list[MMG2_LONMAX],lon;
  int     k,kk;

  for(k=1; k<=mesh->np-4; k++) {
    ppt = &mesh->point[k];

    /* Find the triangle lel of the mesh containing ppt */
    list[0] = MMG2_findTria(mesh,k);

    /* Exhaustive search if not found */
    if ( !list[0] ) {
      if ( mesh->info.ddebug )
        printf(" ** exhaustive search of point location.\n");

      for(kk=1; kk<=mesh->nt; kk++) {
        list[0] = MMG2_isInTriangle(mesh,kk,&ppt->c[0]);
        if ( list[0] ) break;
      }
      if ( kk>mesh->nt ) {
        fprintf(stdout,"  ## Error: no triangle found for vertex %d\n",k);
        return(0);
      }
    }

    /* Create the cavity of point k starting from list[0] */
    lon = _MMG2_cavity(mesh,sol,k,list);

    if ( lon < 1 ) {
      fprintf(stdout,"  ## Error: unable to insert vertex %d\n",k);
      return(0);
    }

    else {
      _MMG2_delone(mesh,sol,k,list,lon);
    }
  }

  return(1);
}

/** Put different references on different subdomains */
int MMG2_markSD(MMG5_pMesh mesh) {
  MMG5_pTria   pt,pt1;
  MMG5_pEdge   ped;
  MMG5_pPoint  ppt;
  int          k,l,iadr,*adja,ped0,ped1,*list,ipil,ncurc,nref;
  int          kinit,nt,nsd,ip1,ip2,ip3,ip4,ned,iel,voy;
  char         i,j,i1,i2;

  /* Reset flag field for triangles */
  for(k=1 ; k<=mesh->nt ; k++)
    mesh->tria[k].flag = mesh->mark;

  _MMG5_SAFE_CALLOC(list,mesh->nt,int);
  kinit = 0;
  nref  = 0;
  ip1   =  mesh->np;

  /* Catch first triangle with vertex ip1 */
  for(k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    pt->flag = mesh->mark;
    pt->ref  = 0;
    if ( (!kinit) && ( pt->v[0]==ip1 || pt->v[1]==ip1 || pt->v[2]==ip1) ) kinit = k;
  }

  /* Travel mesh by adjacencies to set references on triangles as long as no boundary is met */
  do {
    nref++;
    list[0] = kinit;
    ipil = 0;
    ncurc = 0;
    do {
      k = list[ipil];
      pt = &mesh->tria[k];
      pt->ref = nref;
      iadr = 3*(k-1) + 1;
      adja = &mesh->adja[iadr];
      for(i=0; i<3; i++) {
        iel = adja[i] / 3;
        pt1 = &mesh->tria[iel];

        if( !iel || pt1->ref == nref ) continue;

        i1 = _MMG5_inxt2[i];
        i2 = _MMG5_iprv2[i];
        ped0 = pt->v[i1];
        ped1 = pt->v[i2];

/* WARNING: exhaustive search among edges, to be optimized with a hashing structure */
        for(l=1; l<=mesh->na; l++) {
          ped = &mesh->edge[l];
          if( ( ped->a == ped0 && ped->b == ped1 ) || ( ped->b == ped0 && ped->a == ped1 ) ) break;
        }
        if ( l <= mesh->na ) continue;

        pt1->ref = nref;
        list[++ncurc] = iel;
      }
      ++ipil ;
    }
    while ( ipil <= ncurc );

    kinit = 0;
    for(k=1; k<=mesh->nt; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) ) continue;
      pt->flag = mesh->mark;
      if ( !kinit && !(mesh->tria[k].ref) ) kinit = k;
    }
  }
  while ( kinit );

  /* nref - 1 subdomains because Bounding Box triangles have been counted */
  fprintf(stdout," %8d SUB-DOMAINS\n",nref-1);

  _MMG5_SAFE_FREE(list);

  /* Remove BB triangles and vertices */
  /*BB vertex*/
  ip1=(mesh->np-3);
  ip2=(mesh->np-2);
  ip3=(mesh->np-1);
  ip4=(mesh->np);

  /* Case when there are inner and outer triangles */
  if ( nref != 1 ) {
    nt = mesh->nt;
    for(k=1; k<=nt; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) ) continue;
      for(i=0; i<3; i++)
        mesh->point[pt->v[i]].tag = 0;

      if ( pt->ref != 1 ) continue;
      /*update adjacencies*/
      iadr = 3*(k-1)+1;
      adja = &mesh->adja[iadr];
      for(i=0; i<3; i++) {
        if ( !adja[i] ) continue;
        iel = adja[i] / 3;
        voy = adja[i] % 3;
        (&mesh->adja[3*(iel-1)+1])[voy] = 0;
      }
      _MMG2D_delElt(mesh,k);
    }
  }
  /* Remove only the triangles containing one of the BB vertex*/
  else {
    nt = mesh->nt;
    for(k=1 ; k<=nt ; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) ) continue;
      for(i=0; i<3; i++)
        mesh->point[pt->v[i]].tag = 0;
      if( !(pt->v[0]==ip1 || pt->v[1]==ip1 || pt->v[2]==ip1 ||
           pt->v[0]==ip2 || pt->v[1]==ip2 || pt->v[2]==ip2 ||
           pt->v[0]==ip3 || pt->v[1]==ip3 || pt->v[2]==ip3 ||
           pt->v[0]==ip4 || pt->v[1]==ip4 || pt->v[2]==ip4 ) ) continue;

      /*update adjacencies*/
      iadr = 3*(k-1)+1;
      adja = &mesh->adja[iadr];
      for(i=0 ; i<3 ; i++) {
        if(!adja[i]) continue;
        iel = adja[i]/3;
        voy = adja[i]%3;
        (&mesh->adja[3*(iel-1)+1])[voy] = 0;
      }
      _MMG2D_delElt(mesh,k);
    }
  }

  _MMG2D_delPt(mesh,ip1);
  _MMG2D_delPt(mesh,ip2);
  _MMG2D_delPt(mesh,ip3);
  _MMG2D_delPt(mesh,ip4);

  if(mesh->info.renum) {
    nsd = mesh->info.renum;
    nt = mesh->nt;
    for(k=1 ; k<=nt ; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) ) continue;
      pt->ref--;
      if ( pt->ref == nsd ) continue;
      _MMG2D_delElt(mesh,k);
    }
  }

  /* Remove vertex*/
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !M_EOK(pt) )  continue;
    for (i=0; i<3; i++) {
      ppt = &mesh->point[ pt->v[i] ];
      ppt->tag &= ~MG_NUL;
    }
  }
  /* Remove edge*/
  ned = mesh->na;
  for (k=1; k<=ned; k++) {
    ped = &mesh->edge[k];
    if ( !ped->a )  continue;
    ppt = &mesh->point[ ped->a ];
    if ( !M_VOK(ppt) ) {
      _MMG5_delEdge(mesh,k);
      continue;
    }
    ppt = &mesh->point[ ped->b ];
    if ( !M_VOK(ppt) ) {
      _MMG5_delEdge(mesh,k);
      continue;
    }
  }

  return(1);
}

/**
 * \param mesh pointer toward the mesh structure.
 * \param sol pointer toward the sol structure.
 * \return 0 if fail, 1 if success.
 *
 * Mesh triangulation.
 *
 **/
int MMG2_mmg2d2(MMG5_pMesh mesh,MMG5_pSol sol) {
  MMG5_pTria     pt;
  MMG5_pPoint    ppt,ppt2;
  double    c[2],dd;
  int       j,k,kk,ip1,ip2,ip3,ip4,jel,kel,nt,iadr,*adja;
  int       *numper;

  mesh->base = 0;
  /* If triangles already exist, delete them */
  if ( mesh->nt ) {
    nt = mesh->nt;
    for(k=1 ; k<=nt ; k++) {
      _MMG2D_delElt(mesh,k);
      iadr = 3*(k-1) + 1;
      adja = &mesh->adja[iadr];
      adja[0] = 0;
      adja[1] = 0;
      adja[2] = 0;
    }
  }

  /* This part seems useless */
  /* Deal with periodic vertices */
  if ( mesh->info.renum == -10 ) {
    _MMG5_SAFE_CALLOC(numper,mesh->np+1,int);
    for (k=1; k<=mesh->np; k++) {
      ppt = &mesh->point[k];
      for (kk=k; kk<=mesh->np; kk++) {
        if(k==kk) continue;
        ppt2 = &mesh->point[kk];
        dd = (ppt->c[0]-ppt2->c[0])*(ppt->c[0]-ppt2->c[0])+(ppt->c[1]-ppt2->c[1])*(ppt->c[1]-ppt2->c[1]);
        if ( dd < 1.e-6 ) {
          //printf("point img %d %d\n",k,kk);
          ppt2->tmp = 1;
          if ( !numper[k] ) {
            numper[k] = kk;
          }
          else if ( numper[k]!=kk ){
            j = numper[k];
            // printf("j = %d %d\n",j,numper[j]) ;
            while(numper[j] && numper[j]!=kk) {
              j = numper[j];
              //printf("j = %d %d\n",j,numper[j]) ;
            }
            if(numper[j]!=kk) numper[j] = kk;
          }
        }
      }
    }
  }

  /* Create the 4 vertices of the bounding box */
  /* Bottom left corner */
  c[0] = -0.5; //mesh->info.min[0] - 1.;
  c[1] = -0.5; // mesh->info.min[1] - 1.;
  ip1 = _MMG2D_newPt(mesh,c,0);
  if ( !ip1 ) {
    /* reallocation of point table */
    _MMG2D_POINT_REALLOC(mesh,sol,ip1,mesh->gap,
                         printf("  ## Error: unable to allocate a new point\n");
                         _MMG5_INCREASE_MEM_MESSAGE();
                         return(-1)
                         ,c,0);
  }

  /* Top left corner */
  c[0] = -0.5; //mesh->info.min[0] - 1.;
  c[1] =  _MMG2D_PRECI / mesh->info.delta *(mesh->info.max[1]-mesh->info.min[1]) + 0.5;//mesh->info.max[1] + 1.;
  ip2 = _MMG2D_newPt(mesh,c,0);
  if ( !ip2 ) {
    /* reallocation of point table */
    _MMG2D_POINT_REALLOC(mesh,sol,ip2,mesh->gap,
                         printf("  ## Error: unable to allocate a new point\n");
                         _MMG5_INCREASE_MEM_MESSAGE();
                         return(-1)
                         ,c,0);
  }

  /* Bottom right corner */
  c[0] =  _MMG2D_PRECI / mesh->info.delta *(mesh->info.max[0]-mesh->info.min[0]) + 0.5;//mesh->info.max[0] + 1.;
  c[1] = -0.5;//mesh->info.min[1] - 1.;
  ip3 = _MMG2D_newPt(mesh,c,0);
  if ( !ip3 ) {
    /* reallocation of point table */
    _MMG2D_POINT_REALLOC(mesh,sol,ip3,mesh->gap,
                         printf("  ## Error: unable to allocate a new point\n");
                         _MMG5_INCREASE_MEM_MESSAGE();
                         return(-1)
                         ,c,0);
  }

  /* Top right corner */
  c[0] =  _MMG2D_PRECI / mesh->info.delta *(mesh->info.max[0]-mesh->info.min[0]) + 0.5;//mesh->info.max[0] + 1.;
  c[1] = _MMG2D_PRECI / mesh->info.delta *(mesh->info.max[1]-mesh->info.min[1]) + 0.5;//mesh->info.max[1] + 1.;
  ip4 = _MMG2D_newPt(mesh,c,0);
  if ( !ip4 ) {
    /* reallocation of point table */
    _MMG2D_POINT_REALLOC(mesh,sol,ip4,mesh->gap,
                         printf("  ## Error: unable to allocate a new point\n");
                         _MMG5_INCREASE_MEM_MESSAGE();
                         return(-1)
                         ,c,0);
  }

  assert ( ip1 == mesh->np-3 );
  assert ( ip2 == mesh->np-2 );
  assert ( ip3 == mesh->np-1 );
  assert ( ip4 == mesh->np );

  /* Create the first two triangles in the mesh and the adjacency relations */
  jel  = _MMG2D_newElt(mesh);
  if ( !jel ) {
    _MMG2D_TRIA_REALLOC(mesh,jel,mesh->gap,
                       printf("  ## Error: unable to allocate a new element.\n");
                       _MMG5_INCREASE_MEM_MESSAGE();
                       printf("  Exit program.\n");
                       exit(EXIT_FAILURE));
  }
  pt       = &mesh->tria[jel];
  pt->v[0] = ip1;
  pt->v[1] = ip4;
  pt->v[2] = ip2;
  pt->base = mesh->base;

  kel  = _MMG2D_newElt(mesh);
  if ( !kel ) {
    _MMG2D_TRIA_REALLOC(mesh,kel,mesh->gap,
                       printf("  ## Error: unable to allocate a new element.\n");
                       _MMG5_INCREASE_MEM_MESSAGE();
                       printf("  Exit program.\n");
                       exit(EXIT_FAILURE));
  }
  pt   = &mesh->tria[kel];
  pt->v[0] = ip1;
  pt->v[1] = ip3;
  pt->v[2] = ip4;
  pt->base = mesh->base;

  iadr = 3*(jel-1) + 1;
  adja = &mesh->adja[iadr];
  adja[2] = 3*kel + 1;

  iadr = 3*(kel-1) + 1;
  adja = &mesh->adja[iadr];
  adja[1] = 3*jel + 2;

  /* Insertion of vertices in the mesh */
  if ( !MMG2_insertpointdelone(mesh,sol) ) return(0);

  fprintf(stdout,"  -- END OF INSERTION PHASE\n");

  /* Enforcement of the boundary edges */
  if ( !MMG2_bdryenforcement(mesh,sol) ) {
    printf("  ## Error: unable to enforce the boundaries.\n");
    return(0);
  }

  if(mesh->info.ddebug)
    if ( !_MMG5_chkmsh(mesh,1,0) ) exit(EXIT_FAILURE);

  /* Mark SubDomains and remove the bounding box triangles */
  if ( mesh->na )
    MMG2_markSD(mesh);
  else {
    /* Tag triangles : in = base ; out = -base ; Undetermined = 0*/
    if ( !MMG2_settagtriangles(mesh,sol) ) return(0);
    if ( !MMG2_removeBBtriangles(mesh) ) return(0);
  }

  return(1);
}
