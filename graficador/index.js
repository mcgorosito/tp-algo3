
let graphG,graphH;

function generateGraphs(){
  let coloring = document.getElementById("graphInput")["coloring"].value.split(' ');
  let groupColor = coloring.map((c) => {return {"group": c}});
  let graphs = document.getElementById("graphInput")["graphs"].value.split(' ');
  let n = parseInt(graphs[0], 10);
  let mG = parseInt(graphs[1], 10);
  let mH = parseInt(graphs[2], 10);
  let graphG = graphs.slice(3,3+mG*2);
  let graphH = graphs.slice(3+mG*2);
  graphG = graphG.map((e) => e-1);
  graphH = graphH.map((e) => e-1);
  let edgesG = [];
  graphG.forEach((e,index) => {
      if(index%2==0){
        edgesG.push({"from":graphG[index],"to":graphG[index+1]});
      }
  });
  let edgesH = [];
  graphH.forEach((e,index) => {
      if(index%2==0){
        edgesH.push({"from":graphH[index],"to":graphH[index+1]});
      }
  });

  let modelG = {
    nodes: groupColor,
    edges: edgesG
  };

  let modelH = {
    nodes: groupColor,
    edges: edgesH
  };

  drawGraphs(modelG,modelH);
}

function drawGraphs(modelG,modelH){

  console.log(modelG);
  console.log(modelH);

  graphG = new ElGrapho({
    container: document.getElementById('containerG'),
    model: ElGrapho.layouts.Cluster(modelG),
    width: 500,
    height: 500
  });

  graphH = new ElGrapho({
    container: document.getElementById('containerH'),
    model: ElGrapho.layouts.Cluster(modelH),
    width: 500,
    height: 500
  });
}
