#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <math.h> 
#include <set>
#include <map>
#define DBG(x) cerr << #x << " = " << (x) <<"\n"

using namespace std;

//=============== Defines para más declaración
constexpr int INFTY = -1*(1 << 28);
constexpr int UNDEFINED = -1;

using Vertice = int;
using Grafo = vector<vector<Vertice>>; // Representacion del grafo con listas de adyacencia 
using Coloreo = vector<Vertice>; // El i-ésimo elemento del vector representa el color que tiene el i-ésimo vertice en el grafo
using Arista = pair<int,int>;
struct Solucion {
    pair<int,int> indiceDeSwap; // Change: <indice, color>. Swap: <indice, indice>
	pair<int,int> aplicacionDeChange;
	Coloreo coloreo;  // Coloreo resultado

	//constructor default
	Solucion(){
		indiceDeSwap = make_pair(UNDEFINED, UNDEFINED);
		aplicacionDeChange = make_pair(UNDEFINED, UNDEFINED);
		coloreo = Coloreo();
	}
};

//============== Variables globales
int cantVertices;
int cantAristasH;
int cantAristasG;
bool memoriaUltimasSoluciones; //Tipo de tabu search
bool vecindadPorColoreoEnH;
vector<Solucion> memoriaTabu;
int tamanoMemoria;
int porcentajeSubvecindad;
//criterios de corte
int limiteIteraciones;
int limiteIteracionesSinMejora; 

// G : grafo sobre el que debe realizarse el coloreo
Grafo G;
// H : grafo sobre el que debe analizarse el impacto del coloreo anterior
Grafo H;
Coloreo ordenDeColores; // Inicializado antes de cada algoritmo con la cantidad de vertices y todos los valores en -1

//===================================================== AUXILIARES ==========================================================
int impactoDeColoreo(Coloreo& colores) {
	int impacto = 0;
	// Buscamos las aristas que coinciden en color
	for(int j = 0; j < cantVertices; j++) //O(mG + mH)
		for(long unsigned int k = 0; k < H[j].size(); k++)		//NOTA : esto tal vez con la matriz de adyacencia hubise sido mucho mas facil
			if(colores[j] == colores[H[j][k]-1]) impacto++;
	return impacto/2;
}

//dado un coloreo, verifica si es válido. 
//debug : si es true se generan prints, sino no
bool verificadorColoreo(Coloreo coloreo, bool debug){
	bool coloreoInvalido = false;
	 
	for(int i = 0 ; i < cantVertices ; i++)
		for(long unsigned j = 0 ; j < G[i].size() ; j++ )
			if(ordenDeColores[i] == ordenDeColores[G[i][j]-1]){
				if(debug)cout<<"Hay un coloreo inválido en los vertices :"<<i+1<<" y "<<G[i][j]<<endl;
				coloreoInvalido = true;
			}
	if(coloreoInvalido){

		cout << "Grafo G :"<<endl;
		for(int j = 0 ; j < cantVertices ; j++){ cout << "vertice :"<< j+1 <<endl; 
			for(long unsigned int k = 0 ; k < G[j].size() ; k++){ cout<<G[j][k]<<" ";}cout<<endl;
		}
		
		cout << "Grafo H :"<<endl;
		for(int j = 0 ; j < cantVertices ; j++){ 
			cout << "vertice :"<< j+1 <<endl; for(long unsigned int k = 0 ; k < H[j].size() ; k++){cout<<H[j][k]<<" ";
			}cout<<endl;
		}
		cout<<"Coloreo obtenido :"<<endl;
		for(int i = 0; i < cantVertices; i++){cout << ordenDeColores[i] << " ";} cout<<endl;
	}
	return coloreoInvalido;
}

// Retorna el vertice que comparten las aristas, en caso de no compartir ninguno, retornamos UNDEFINED
Vertice verticeCompartido(Arista a, Arista b) {
	if(a.first == b.first || a.first == b.second)
		return a.first;
	else if(a.second == b.first || a.second == b.second)
		return a.second;
	else
		return UNDEFINED;
}

bool seEncuentra(Vertice vertice, vector<Vertice> vertices) {
//Chequea si el segundo vertice pertenece al vector de adyacencia
	long unsigned i = 0;
	while(i < vertices.size() && vertices[i]!=vertice) i++;
	return i < vertices.size();	
}

bool aristaYaAgregada(vector<Arista> aristas, Arista arista){
	long unsigned i = 0;
	while(i < aristas.size() && (aristas[i].first!=arista.first || aristas[i].second!=arista.second))i++;

	return i < aristas.size();
}

bool colorValido(Vertice vertice, int color){
	long unsigned j = 0 ;
	while( j < G[vertice-1].size() && color != ordenDeColores[G[vertice-1][j]-1])j++;
	return j == G[vertice-1].size();
}

bool comparacionPorGradoH(const int& a, const int& b) {
	return H[a-1].size() > H[b-1].size() || (H[a-1].size() == H[b-1].size() && a >= b);
}

bool comparacionPorGradoHAristas(const Arista& a, const Arista& b) {
	long unsigned mayorGradoA = H[a.first-1].size() > H[a.second-1].size() ? H[a.first-1].size() : H[a.second-1].size();
	long unsigned mayorGradoB = H[b.first-1].size() > H[b.second-1].size() ? H[b.first-1].size() : H[b.second-1].size();
	return mayorGradoA > mayorGradoB || (mayorGradoA == mayorGradoB && a > b);
}

bool comparacionPorGradoG(const int& a, const int& b) {
	return G[a-1].size() > G[b-1].size() || (G[a-1].size() == G[b-1].size() && a >= b);
}
//Parametros:	
// orden : vector<vertice> donde los vertices representan el orden de como se ejecuta la heuristica y su coloreo asociado
// output : maximo color utilizado
int heuristicaSecuencial(vector<Vertice>& orden) {
	// Inicializo el vector que indica los colores usados y sin usar
	int maxColor = -1;
	vector<bool> coloresSinUsar(cantVertices, false);
	Vertice v;
	int cr;
	// Recorro el grafo
	for (int i = 0; i < cantVertices; i++) { // O(n+m)
		v = orden[i];
		// Todos los colores asignados en la iteración anterior se ponen como "en uso" (true)	
		for (long unsigned int j = 0; j < G[v-1].size(); j++)
			if (ordenDeColores[G[v-1][j]-1] != UNDEFINED)
				coloresSinUsar[ordenDeColores[G[v-1][j]-1]] = true;
		// Se utiliza el primer color sin utilizar para colorear el vertice
		cr = 0;
		while (cr < cantVertices && coloresSinUsar[cr])
			cr++;
		if(ordenDeColores[v-1]==UNDEFINED)ordenDeColores[v-1] = cr;
		// Actualizamos el color maximo utilizado hasta ahora
		if(cr > maxColor) maxColor = cr;
   }
   return maxColor;
}
void busquedaLocalGolosa(int maxColor, int mayorImpacto){
	// Heuristica de busqueda local
	for(int j = 0; j < cantVertices; j++) { 
		for(long unsigned int k = 0; k < H[j].size(); k++) {
			if(ordenDeColores[j] != ordenDeColores[H[j][k]-1]) {
				// Busca en la misma arista en G
				long unsigned int i = 0;
				while(i < G[j].size() && G[j][i]!=H[j][k]) i++;
				int colorAnteriorColores = ordenDeColores[j];
				int colorAnteriorGrafoH = ordenDeColores[H[j][k]-1];
				// Si la arista no estaba en G y puedo agregar un color mas lo hago y pruebo el impacto
				if(G[j].size() == i && maxColor + 1 < cantVertices){
					ordenDeColores[H[j][k]-1] = maxColor+1;
					ordenDeColores[j] = maxColor + 1;
					maxColor++;
				}
				// Si el impacto luego de agregar el color es menor o igual, entonces lo dejamos como estaba
				int impactoEncontrado = impactoDeColoreo(ordenDeColores);
				if(impactoEncontrado > mayorImpacto) {
					mayorImpacto = impactoEncontrado;
				} else {
					ordenDeColores[H[j][k]-1] = colorAnteriorGrafoH;
					ordenDeColores[j] = colorAnteriorColores;
					maxColor--;
				}
			}
		}
	}
}
void coloreoVerticeSegunH(vector<Vertice> verticesAColorear, int& cr){
	int color = UNDEFINED;
	long unsigned i = 0;
	Vertice ultimo = verticesAColorear[verticesAColorear.size()-1];
	while(i < verticesAColorear.size()-1 && color == UNDEFINED){
		Vertice iesimo = verticesAColorear[i];
		//si son adyacentes intento colorearlos con el mismo color
		if(seEncuentra(iesimo, H[ultimo-1])){
			//si iesimo ya tiene asigando un color intentamos ponerle el mismo color
			if( ordenDeColores[iesimo-1]!=UNDEFINED && colorValido(ultimo, ordenDeColores[iesimo-1]))
				color = ordenDeColores[iesimo-1];
			
			
		}
		i++;
	}
	// Si no compartia vertice con ninguno, le pongo un color nuevo
	ordenDeColores[ultimo-1] = color == UNDEFINED ? cr : color;
	if(color == UNDEFINED)cr++;	
}
void coloreoAristaSegunH(vector<Arista> aristasQueNoEstanEnG,Arista ultimaAristaAgregada, int& cr){
	int color = UNDEFINED;
	long unsigned i = 0;
	Vertice restante = UNDEFINED;
	while(i < aristasQueNoEstanEnG.size()-1 && color == UNDEFINED){
		Arista iesima = aristasQueNoEstanEnG[i];
		Vertice compartido = verticeCompartido(iesima, ultimaAristaAgregada);
		// Si comparten un vertice
		if(compartido != UNDEFINED){
			//vamos a ver si lo podemos colorear con el mismo color y resulta válido en G
			restante = compartido != ultimaAristaAgregada.first ? ultimaAristaAgregada.first : ultimaAristaAgregada.second;
			//si el color del compartido es valido le ponemos este color
			if(colorValido(restante,ordenDeColores[compartido-1]))
				color = ordenDeColores[compartido - 1];

		}
		i++;
	}
	// Si no compartia vertice con ninguno, le pongo un color nuevo
	if(color == UNDEFINED){
		ordenDeColores[ultimaAristaAgregada.first-1] = cr;
		ordenDeColores[ultimaAristaAgregada.second-1] = cr;
		cr++;
	}else
		ordenDeColores[restante-1] = color;
}

//evalua si se cumple el criterio de corte por iteraciones sin mejora o iteraciones
bool continuarTabuSearch(int iteracionesSinMejora, int iteraciones){ 
	return iteracionesSinMejora < limiteIteracionesSinMejora || iteraciones < limiteIteraciones;
}

bool mismaEstructuraDeSolucion(Solucion a, Solucion b){
	if(memoriaUltimasSoluciones){
		return a.coloreo == b.coloreo;
	}
	else
		return a.aplicacionDeChange == b.aplicacionDeChange && a.indiceDeSwap == b.indiceDeSwap;
}
bool solucionExplorada(Solucion solucion){
	long unsigned j = 0; 
	while (j < memoriaTabu.size() ){
		if(mismaEstructuraDeSolucion(solucion, memoriaTabu[j]))
			break;
		j++;
	} 
	return j < memoriaTabu.size();	
}

void vecindadPorSwap(vector<Solucion>& vecindad, Solucion solucion){
	
	for(int i = 0 ; i < cantVertices - 1; i++){
		for(int j = i+1 ; j < cantVertices ; j++){
			swap(solucion.coloreo[i], solucion.coloreo[j]);
			solucion.indiceDeSwap = !memoriaUltimasSoluciones && !solucionExplorada(solucion) ? make_pair(j, i) :  make_pair(UNDEFINED,UNDEFINED) ;
			vecindad.push_back(solucion);
			swap(solucion.coloreo[j], solucion.coloreo[i]);
		}
	}
}

void vecindadPorAristasEnH(vector<Solucion>& vecinos, Solucion solOriginal){
	
	vector<bool> coloresUsados = vector<bool>(cantVertices, false);
	for(int i = 0; i < cantVertices; i++)coloresUsados[solOriginal.coloreo[i]]=true;
	//recorremos las aristas de H
	for(int j = 0; j < cantVertices; j++) { 
		for(long unsigned int k = 0; k < H[j].size(); k++) {
			//intentamos colorear aristas de H del mismo color
			if(solOriginal.coloreo[j] != solOriginal.coloreo[H[j][k]-1]) {
				// Si no son adyacentes en G
				if(!seEncuentra(H[j][k],G[j])){
					int colorAnteriorGrafoH = solOriginal.coloreo[H[j][k]-1];
					int colorAnteriorColores = solOriginal.coloreo[j];

					if( colorValido(j+1,solOriginal.coloreo[H[j][k]-1]))
						solOriginal.coloreo[j] = solOriginal.coloreo[H[j][k]-1];

					//intento colorear ambos vertices del mismo color sin agregar uno nuevo
					else if (colorValido(H[j][k],solOriginal.coloreo[j]))
						solOriginal.coloreo[H[j][k]-1] = solOriginal.coloreo[j];
					//si no podemos colorearlas sin usar un color nuevo intentamos agregar un color nuevo
					if(solOriginal.coloreo[j] != solOriginal.coloreo[H[j][k]-1]){
						//coloreamos ambos extremos de un color que no se haya utilizado
						int color = 0;
						while( color < cantVertices && coloresUsados[color])color++;
						solOriginal.coloreo[j] = color;
						solOriginal.coloreo[H[j][k]-1] = color;
					}
					//agrego la solucion con la arista del mismo color
					vecinos.push_back(solOriginal);
					
					solOriginal.coloreo[H[j][k]-1] = colorAnteriorGrafoH;
					solOriginal.coloreo[j] = colorAnteriorColores;
					
				}
			}
		}
	}
}

vector<Solucion> seleccionoSubvecindadPorRandom(vector<Solucion> vecinos){
	int elementosASeleccionar = floor(vecinos.size()*(porcentajeSubvecindad / 100));
	vector<Solucion> subVecindad(elementosASeleccionar, Solucion());
	// genera un random entre 0 and cantVertices : 
	srand (time(NULL));
	int indice;
	int seleccionado = 0;
	while(seleccionado <= elementosASeleccionar){
		indice = rand() % elementosASeleccionar;
		if(vecinos[indice].coloreo.size() != 0){
			//extraigo la solucion
			subVecindad[seleccionado] = vecinos[indice];
			//anulo la solucion extraida para no seleccionarla nuevamente
			vecinos[indice] = Solucion();
			seleccionado++;

		}
	}
	return subVecindad;
	
}

Solucion aplicoChange(Solucion solucion) {
	//este vector va a ser utilizado para indicar que colores estan siendo utilizados en la solucion actual
	vector<bool> coloresUsados = vector<bool>(cantVertices, false);
	Coloreo nuevoColoreo = solucion.coloreo;
	//inicializo la semilla
	srand (time(NULL));
	// genera un random entre 0 and cantVertices : 
	int indice = rand() % cantVertices;
	//indico que colores estan siendo utilizados
	for(int i = 0; i < cantVertices; i++)coloresUsados[solucion.coloreo[i]]=true;
	

	//pongo el primer color sin utilizar en el indice obtenido aleatoriamente		
	int j = 0;
	while( j < cantVertices && coloresUsados[j])j++;
	nuevoColoreo[indice]=j;
	
	//en el caso de que todos los colores esten utilizados devuelvo la solucion sin aplicar change
	if(j==cantVertices) 
		return solucion;

	solucion.coloreo = nuevoColoreo;
	solucion.aplicacionDeChange = make_pair(indice, j+1);
	solucion.indiceDeSwap = make_pair(UNDEFINED, UNDEFINED);
	return solucion;
}

vector<Solucion> subVecindad(Solucion solOriginal, bool conChange, bool vecindadPorColoreoEnH){
	vector<Solucion> vecinos = vector<Solucion>();
	if(porcentajeSubvecindad > 100 || porcentajeSubvecindad < 0) {
		cout<<"ingresó un porcentaje a explorar inválido."<<endl;
		return vecinos;
	}
	if(!vecindadPorColoreoEnH){
		if(conChange) 
			solOriginal = aplicoChange(solOriginal); 
		// Propuesta: yo lo que haría seria aplicar todos los swaps posibles (orden cuadrático) explorarlos y luego aplicar un change, y así sucesivamente
		vecindadPorSwap(vecinos, solOriginal);
	}else
		vecindadPorAristasEnH(vecinos, solOriginal);

	//retorno subvecindad
	return seleccionoSubvecindadPorRandom(vecinos);
}


Solucion mejorOrdenEntreVecinos(vector<Solucion> vecinos, Solucion solucionActual) {
	Solucion mejorVecino;
	int mejorImpacto = -1;
	int impactoVecinoActual;
	for (long unsigned int i = 0; i < vecinos.size(); i++){
	
		if(!solucionExplorada(vecinos[i])) {
			impactoVecinoActual = impactoDeColoreo(vecinos[i].coloreo);
			if(impactoVecinoActual > mejorImpacto) {
				mejorVecino = vecinos[i];
				mejorImpacto = impactoVecinoActual;
			}
		}
	}
	return mejorVecino;
}



//================================================= FIN AUXILIARES ==========================================================
//como la heurística tiene que ser solamente constructiva, usamos el parametro bool en default false. Pero lo dejamos para utilizarlo en la experimentacion
int primerHeuristica(vector<Vertice> ordenVertices, bool conBusquedaLocal, bool porGradoG) {
		// Inicializamos los vertices que vamos a usar para representar el orden
		for(int i = 1; i < cantVertices + 1; i++)ordenVertices[i-1] = i;
		//sort(ordenVertices.begin(), ordenVertices.end(), customComparison); //ordenamos segun criterio mencionado para la utilizacion de la heuristica secuencial
		if(porGradoG)
			sort(ordenVertices.begin(), ordenVertices.end(), comparacionPorGradoG);
		else
			sort(ordenVertices.begin(), ordenVertices.end(), comparacionPorGradoH);
		// Ver complejidad del sort en std
		// Realizamos el coloreo de manera secuencial segun orden de vertices por adyacencia en G y desempatando por adyacencia en H de mayor a menor 
		int maxColor = heuristicaSecuencial(ordenVertices);
		//Si es con busqueda local, me deja en ordenDeColores el mejor que encontro
		if(conBusquedaLocal) {
			int mayorImpacto = impactoDeColoreo(ordenDeColores);
			busquedaLocalGolosa(maxColor, mayorImpacto);
		}
		
		return impactoDeColoreo(ordenDeColores);
		
}

// Dada una arista en H buscamos que los vertices en sus extremos coincidan en color con G
int segundaHeuristica() {
	// Agarramos todas las aristas de H que no esten en G y las almaceno en un vector de aristas
	vector<Arista> aristasQueNoEstanEnG = vector<Arista>();
	int cr = 0;
	for(int j = 0; j < cantVertices; j++) { // O(cantVertices * (mG+mH))
		for(long unsigned int k = 0; k < H[j].size(); k++) {
			// Si esta arista no esta en G
			if(!seEncuentra(H[j][k],G[j]) && cr < cantVertices) {
				aristasQueNoEstanEnG.push_back(make_pair(j+1, H[j][k]));
				sort(aristasQueNoEstanEnG.begin(),aristasQueNoEstanEnG.end(),comparacionPorGradoHAristas);
				Arista ultimaAristaAgregada = aristasQueNoEstanEnG[aristasQueNoEstanEnG.size()-1];
				// Coloreamos los vertices de la arista de manera que sea valido ese coloreo en G pero tengan el mismo color
				// Si es imposible ese coloreo ponemos un color nuevo en el vertice que menos grado tiene para reducir el impacto perdido y a los restantes les ponemos el mismo color
				coloreoAristaSegunH(aristasQueNoEstanEnG,ultimaAristaAgregada, cr);
			}
		}
	}
    vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
    for(int i = 1; i < cantVertices + 1; i++)ordenVertices[i-1] = i;		
    heuristicaSecuencial(ordenVertices); 
    
    return impactoDeColoreo(ordenDeColores);
}

// Dada una arista en H buscamos que los vertices en sus extremos coincidan en color con G
int tercerHeuristica() {
	//obtengo los vertices que son incidentes a alguna arista en H
	vector<Vertice> verticesConAristasEnH = vector<Vertice>();
	for(int i = 0; i < cantVertices; i++)if(H[i].size() > 0)verticesConAristasEnH.push_back(i+1);
	//los ordeno segun su grado
	sort(verticesConAristasEnH.begin(),verticesConAristasEnH.end(),comparacionPorGradoH);
	vector<Vertice> verticesAColorear = vector<Vertice>();
	
	int cr = 0;
	for(long unsigned j = 0; j < verticesConAristasEnH.size(); j++) { 
		Vertice v = verticesConAristasEnH[j]; 
		if(!seEncuentra(v, verticesAColorear)){

			verticesAColorear.push_back(v);
			ordenDeColores[v-1]=cr;
			coloreoVerticeSegunH(verticesAColorear, cr);
			for(long unsigned int k = 0; k < H[v-1].size(); k++) {
				Vertice kesimo = H[v-1][k]; 
				// Si esta arista no esta en G
				if(!seEncuentra(kesimo,G[v-1]) && !seEncuentra(kesimo, verticesAColorear) && cr < cantVertices) {
					verticesAColorear.push_back(kesimo);
					coloreoVerticeSegunH(verticesAColorear, cr);
				}
			}
			if(verticesAColorear[verticesAColorear.size()-1]==v){
				verticesAColorear.pop_back();
				ordenDeColores[v-1]=UNDEFINED;
			}
		}
	}
    vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
    for(int i = 1; i < cantVertices + 1; i++)ordenVertices[i-1] = i;		
    heuristicaSecuencial(ordenVertices); 
    
    return impactoDeColoreo(ordenDeColores);
}

int tabuSearch() {
	memoriaTabu.resize(tamanoMemoria, Solucion());                        //Memoria de tamaño definido como global
	vector<Vertice> orden = vector<Vertice>(cantVertices,UNDEFINED);
	int mejorImpacto = tercerHeuristica();      			  //Empezamos con el impacto que nos da la heuristica que elijamos, puede ser en base al gap                              
	int proximoIndexMemoria = 0;
	int iteracionesSinMejora = 0;	//Condicion de salida
	int iteraciones = 0;
	int impactoActual;
	bool aplicoChange = true;
	Solucion solucionActual;
	solucionActual.coloreo = ordenDeColores;   // El orden que nos dio la segunda heuristica
	
	vector<Solucion> solucionesVecinas; // Soluciones "vecinas" a la actual

	while(continuarTabuSearch(iteraciones,iteracionesSinMejora)) { 	// Criterio para parar. Lo dejo modularizado para futuras ideas
		aplicoChange = !aplicoChange;                  
		solucionesVecinas = subVecindad(solucionActual, aplicoChange, vecindadPorColoreoEnH);		//Busco soluciones vecinas
		solucionActual = mejorOrdenEntreVecinos(solucionesVecinas,solucionActual);   // Calculo el mejor entre ellos
		
		//recuerdo la solucion obtenida
		Solucion sol; 
		if(memoriaUltimasSoluciones){
			sol.coloreo = solucionActual.coloreo;
		}else {
			sol.aplicacionDeChange = solucionActual.aplicacionDeChange;
			sol.indiceDeSwap = solucionActual.indiceDeSwap;
		}
		memoriaTabu[proximoIndexMemoria] = sol; 	// Lo guardo en la memoria para futuras exploraciones
		proximoIndexMemoria = (proximoIndexMemoria + 1) % tamanoMemoria; // Avanzamos el proximo slot a pisar
		impactoActual = impactoDeColoreo(solucionActual.coloreo);
		if (impactoActual > mejorImpacto) {               //Si es el mejor, nos lo guardamos
			mejorImpacto = impactoActual;
			iteracionesSinMejora = 0;
		} else {
			iteracionesSinMejora++;
		}
		iteraciones++;
	}
	ordenDeColores = solucionActual.coloreo; // para imprimir al final el coloreo resultante
	return mejorImpacto;
}



// Recibe por parámetro qué algoritmos utilizar para la ejecución separados por espacios.
// Imprime por clog la información de ejecución de los algoritmos.
// Imprime por cout el resultado de algun algoritmo ejecutado.
int main(int argc, char** argv){
	// Leemos el parametro que indica el algoritmo a ejecutar.
	map<string, string> algoritmos_implementados = {
		{"G1", "Primer heuristica golosa constructiva"},{"G1B", "Primer heuristica golosa constructiva, con heuristica de búsqueda local"},
		{"G2", "Segunda heuristica golosa constructiva"},{"G2B", "Segunda heuristica golosa constructiva, con heuristica de búsqueda local"}, 
		{"G3A", "Tercer heuristica golosa coloreando G por las aristas de H."}, {"G3V", "Tercer heuristica golosa coloreando G por los vertices de H."},
		{"M1", "Metaheuristica Tabú search con memoria de vertices"},{"M2", "Metaheuristica Tabú search con memoria de soluciones"},
		{"M1H", "Metaheuristica Tabú search con memoria de vertices con soluciones vecinas alternativas."},{"M2H", "Metaheuristica Tabú search con memoria de soluciones con vecindad alternativas."}
	};

	// Verificar que el algoritmo pedido exista.
	if (argc < 2 || algoritmos_implementados.find(argv[1]) == algoritmos_implementados.end())
	{
		cerr << "Algoritmo no encontrado: " << argv[1] << endl;
		cerr << "Los algoritmos existentes son: " << endl;
		for (auto& alg_desc: algoritmos_implementados) cerr << "\t- " << alg_desc.first << ": " << alg_desc.second << endl;
		return 0;
	}
	string algoritmo = argv[1];
	if(algoritmo[0] == 'M'){
		cin >> tamanoMemoria >> porcentajeSubvecindad >> limiteIteracionesSinMejora >> limiteIteraciones; 
	}
    // Leemos el input.
    cin >> cantVertices >> cantAristasG >> cantAristasH;
    G.assign(cantVertices, vector<int>());
    int i,j;
    for (int k = 0; k < cantAristasG; k++){
    	cin >> i >> j;
		G[i-1].push_back(j);
		G[j-1].push_back(i);	
    }

    H.assign(cantVertices, vector<int>());
    for (int k = 0; k < cantAristasH; k++){
    	cin >> i >> j;
		H[i-1].push_back(j);
		H[j-1].push_back(i);	
    }

	//lo inicializamos con valores basura
	ordenDeColores.assign(cantVertices, -1);
    
	// Ejecutamos el algoritmo y obtenemos su tiempo de ejecución.
	int mayor_impacto = INFTY;
	auto start = chrono::steady_clock::now();
	if (algoritmo == "G1") {
		vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
		mayor_impacto = primerHeuristica(ordenVertices, false, true); 
	}
	else if (algoritmo == "G1B")
	{
		vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
		mayor_impacto = primerHeuristica(ordenVertices, true, true); 
	}
	else if (algoritmo == "G2")
	{
		vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
		mayor_impacto = primerHeuristica(ordenVertices, false, false); 
	}
	else if (algoritmo == "G2B")
	{
		vector<Vertice> ordenVertices = vector<Vertice>(cantVertices, -1); 
		mayor_impacto = primerHeuristica(ordenVertices, true, false); 
	}
	else if (algoritmo == "G3A")
	{
		mayor_impacto = segundaHeuristica();
	}
	else if (algoritmo == "G3V")
	{
		//mayor_impacto = segundaHeuristica();
		mayor_impacto = tercerHeuristica();
	}
	else if (algoritmo == "M1")
	{	
		memoriaUltimasSoluciones = true;
		vecindadPorColoreoEnH = false;
		mayor_impacto = tabuSearch();
	}
	else if (algoritmo == "M2")
	{	
		memoriaUltimasSoluciones = false;
		vecindadPorColoreoEnH = false;
		mayor_impacto = tabuSearch();
	}else if (algoritmo == "M1H")
	{	
		memoriaUltimasSoluciones = true;
		vecindadPorColoreoEnH = true;
		mayor_impacto = tabuSearch();
	}
	else if (algoritmo == "M2H")
	{	
		memoriaUltimasSoluciones = false;
		vecindadPorColoreoEnH = true;
		mayor_impacto = tabuSearch();
	}

	auto end = chrono::steady_clock::now();
	double total_time = chrono::duration<double, milli>(end - start).count();

	// Imprimimos el tiempo de ejecución por stderr.
	std::clog << "tiempo en ms : "<<total_time << endl;

    // Imprimimos el resultado por stdout.
    std::cout << (mayor_impacto == INFTY ? -1 : mayor_impacto) <<endl;
    if(mayor_impacto != INFTY && !verificadorColoreo(ordenDeColores,false)){
    	for(int i = 0 ; i < cantVertices ; i++){
    		std::cout << ordenDeColores[i] << " ";
    	}
		std::cout<<endl;
	}
    else{
		verificadorColoreo(ordenDeColores,true);
    	std::cout<<"No hubo coloreo válido"<<endl;
	}
    
    return 0;
}
/*
codigo para debuggear
	cout << "Grafo H :"<<endl;
	for(int j = 0 ; j < cantVertices ; j++){ //O(mG + mH)
		cout << "vertice :"<< j+1 <<endl; 
		for(long unsigned int k = 0 ; k < H[j].size() ; k++){
			cout<<H[j][k]<<" ";

		}cout<<endl;
	}

	cout << "Grafo G :"<<endl;
	for(int j = 0 ; j < cantVertices ; j++){ cout << "vertice :"<< j+1 <<endl; 
		for(long unsigned int k = 0 ; k < G[j].size() ; k++){ cout<<G[j][k]<<" ";}cout<<endl;
	}
	for(int i = 0; i < cantVertices; i++){cout << ordenDeColores[i] << " ";} cout<<endl;

	for(int i = 0; i < cantVertices; i++){
			cout<< ordenVertices[i] << " ";
	} cout<<endl;
	

//para colorear los restantes de G en la tercer heuristica
// Inicializo el vector que indica los colores usados y sin usar
    vector<bool> coloresSinUsar(cantVertices, false);
	for(int i = 0; i < cantVertices; i++) { // O(n+m)
		// Todos los colores asignados en la iteración anterior se ponen como "en uso" (true)	
		for (long unsigned int j = 0; j < G[i].size(); j++)
			if (ordenDeColores[G[i][j]-1] != UNDEFINED)
				coloresSinUsar[ordenDeColores[G[i][j]-1]] = true;
		// Se utiliza el primer color sin utilizar para colorear el vertice
		while (cr < cantVertices && coloresSinUsar[cr])cr++;
		if(ordenDeColores[i]==UNDEFINED)ordenDeColores[i] = cr;
    }

	./pcmi "G3" < python/instancias/CMI_n12.in ; cat python/instancias/CMI_n12.out 

*/