// This method collects a list of Node names from the current Jenkins instance
@NonCPS
def nodeNames() {
  return jenkins.model.Jenkins.instance.nodes.collect { node -> node.name }
}

def getLabels(String name){
    def computer = Jenkins.getInstance().getComputer(name)
    def node = computer.getNode()
    if(computer.isOnline()){
        return node.getLabelString()
    }
    return ""
}

def names = nodeNames()
//filter nodes
def filtered_names = []
for(n in names){
    if(getLabels(n).contains("logan")){
        filtered_names << n
        print("Got Node: " + n)
    }
}


stage('Checkout'){
    def builders = [:]
    for(n in filtered_names){
        def node_name = n
        builders[node_name] = {
            node(node_name){
            def loganPath = "${LOGAN_PATH}"
                dir(loganPath){
                    checkout([$class: 'GitSCM', branches: [[name: '*/master']], doGenerateSubmoduleConfigurations: false, extensions: [[$class: 'SubmoduleOption', disableSubmodules: false, parentCredentials: true, recursiveSubmodules: false, reference: '', trackingSubmodules: false]], submoduleCfg: [], userRemoteConfigs: [[url: 'https://github.com/cdit-ma/logan.git']]])    
                }
            }
        }
    }
    parallel builders
}

stage('Build'){
    def builders = [:]
    for(n in filtered_names){
        def node_name = n
        builders[node_name] = {
            node(node_name){
                def loganPath = "${LOGAN_PATH}"            
                dir(loganPath + '/build'){
                    sh 'cmake ..'
                    sh 'make -j6'
                }
            }
        }
    }
    parallel builders
}