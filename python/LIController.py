import os
import h5py
import numpy as np

import leptoninjector as LI
from LIDarkNews import PyDarkNewsCrossSectionCollection

# For determining fiducial volume of different experiments
fid_vol_dict = {'MiniBooNE':'fid_vol',
                'CCM':'ccm_inner_argon',
                'MINERvA':'fid_vol'}

# Parent python class for handling event generation
class LIController:

    def __init__(self,
                 events_to_inject,
                 experiment,
                 seed=0):
        """
        LI class constructor.
        :param int event_to_inject: number of events to generate
        :param str experiment: experiment name in string
        :param int seed: Optional random number generator seed
        """

        # Initialize a random number generator
        self.random = LI.utilities.LI_random(seed)
        
        # Save number of events to inject
        self.events_to_inject = events_to_inject
        self.experiment = experiment

        # Empty list for our interaction trees
        self.events = []

        # Find the density and materials files
        LI_SRC = os.environ.get('LEPTONINJECTOR_SRC')
        materials_file = LI_SRC + '/resources/earthparams/materials/%s.dat'%experiment
        if experiment in ['ATLAS','dune']:
            earth_model_file = LI_SRC + '/resources/earthparams/densities/PREM_%s.dat'%experiment
        else:
            earth_model_file = LI_SRC + '/resources/earthparams/densities/%s.dat'%experiment
        
        self.earth_model = LI.detector.EarthModel()
        self.earth_model.LoadMaterialModel(materials_file)
        self.earth_model.LoadEarthModel(earth_model_file)

    def SetProcesses(self,
                     primary_type,
                     primary_injection_distributions,
                     primary_physical_distributions,
                     secondary_types=[],
                     secondary_injection_distributions=[],
                     secondary_physical_distributions=[]):
        """
        LI process setter.
        :param ParticleType primary_type: The primary particle being generated
        :param dict<InjectionDistribution> primary_injection_distributions: The dict of injection distributions for the primary process
        :param dict<PhysicalDistribution> primary_physical_distributions: The dict of physical distributions for the primary process
        :param list<ParticleType> secondary_types: The secondary particles being generated
        :param list<dict<InjectionDistribution> secondary_injection_distributions: List of dict of injection distributions for each secondary process
        :param list<dict<PhysicalDistribution> secondary_physical_distributions: List of dict of physical distributions for each secondary process
        """

        # Define the primary injection and physical process
        self.primary_injection_process = LI.injection.InjectionProcess()
        self.primary_physical_process = LI.injection.PhysicalProcess()
        self.primary_injection_process.primary_type = primary_type
        self.primary_physical_process.primary_type = primary_type

        # Add all injection distributions
        for _,idist in primary_injection_distributions.items():
            self.primary_injection_process.AddInjectionDistribution(idist)
        # Add all physical distributions
        for _,pdist in primary_physical_distributions.items():
            self.primary_physical_process.AddPhysicalDistribution(pdist)
        
        # Default injection distributions
        if 'target' not in primary_injection_distributions.keys():
            self.primary_injection_process.AddInjectionDistribution(LI.distributions.TargetAtRest())
        if 'helicity' not in primary_injection_distributions.keys():
            self.primary_injection_process.AddInjectionDistribution(LI.distributions.PrimaryNeutrinoHelicityDistribution())
    
        # Default injection distributions
        if 'target' not in primary_physical_distributions.keys():
            self.primary_physical_process.AddPhysicalDistribution(LI.distributions.TargetAtRest())
        if 'helicity' not in primary_physical_distributions.keys():
            self.primary_physical_process.AddPhysicalDistribution(LI.distributions.PrimaryNeutrinoHelicityDistribution())

        # Define lists for the secondary injection and physical processes
        self.secondary_injection_processes = []
        self.secondary_physical_processes = []

        # Loop through possible secondary interactions
        for i_sec,secondary_type in enumerate(secondary_types):
            secondary_injection_process = LI.injection.InjectionProcess()
            secondary_physical_process = LI.injection.PhysicalProcess()
            secondary_injection_process.primary_type = secondary_type
            secondary_physical_process.primary_type = secondary_type

             # Add all injection distributions
            for idist in secondary_injection_distributions[i_sec]:
                secondary_injection_process.AddInjectionDistribution(idist)
            # Add all physical distributions
            for pdist in secondary_physical_distributions[i_sec]:
                secondary_physical_process.AddPhysicalDistribution(pdist)
            
            # Add the position distribution
            fid_vol = self.GetFiducialVolume()
            if fid_vol is not None:
                secondary_injection_process.AddInjectionDistribution(LI.distributions.SecondaryPositionDistribution(fid_vol))
            else:
                secondary_injection_process.AddInjectionDistribution(LI.distributions.SecondaryPositionDistribution())
            
            self.secondary_injection_processes.append(secondary_injection_process)
            self.secondary_physical_processes.append(secondary_physical_process)

    def GetFiducialVolume(self):
         # Get fiducial volume
        fid_vol = None
        for sector in self.earth_model.Sectors:
            if self.experiment in fid_vol_dict.keys():
                if sector.name==fid_vol_dict[self.experiment]:
                    fid_vol = sector.geo
        return fid_vol

    def GetEarthModelTargets(self):
        """
        Determines the targets that exist inside the earth model
        :return: lists of targets and strings
        :rtype: (list<ParticleType>, list<str>)
        """
        count = 0
        targets = []
        target_strs = []
        while self.earth_model.Materials.HasMaterial(count):
            for _target in self.earth_model.Materials.GetMaterialTargets(count):
                if _target not in targets:
                    targets.append(_target)
                if str(_target).find('Nucleus') == -1: 
                    continue
                else:
                    target_str = str(_target)[str(_target).find('Type')+5:str(_target).find('Nucleus')]
                    if target_str == 'H': target_str = 'H1'
                    if target_str not in target_strs:
                        target_strs.append(target_str)
            count += 1
        return targets, target_strs
    
    def SetCrossSections(self,
                         primary_cross_section_collection,
                         secondary_cross_section_collections):
        """
        Set cross sections for the primary and secondary processes
        :param CrossSectionCollection primary_cross_section_collection: The cross section collection for the primary process
        :param list<CrossSectionCollection> secondary_cross_section_collections: The list of cross section collections for the primary process
        """
        # Set primary cross sections
        self.primary_injection_process.SetCrossSections(primary_cross_section_collection)
        self.primary_physical_process.SetCrossSections(primary_cross_section_collection)
        
        # Loop through secondary processes
        for sec_inj,sec_phys in zip(self.secondary_injection_processes,
                                    self.secondary_physical_processes):
            assert(sec_inj.primary_type == sec_phys.primary_type)
            record = LI.dataclasses.InteractionRecord()
            record.signature.primary_type = sec_inj.primary_type
            found_collection = False
            # Loop through possible seconday cross sections
            for sec_xs in secondary_cross_section_collections:
                # Match cross section collection on  the primary type
                if sec_xs.MatchesPrimary(record):
                    sec_inj.SetCrossSections(sec_xs)
                    sec_phys.SetCrossSections(sec_xs)
                    found_collection = True
            if(not found_collection):
                print('Couldn\'t find cross section collection for secondary particle %s; Exiting'%record.primary_type)
                exit(0)



    
    def Initialize(self):
        
        # Define stopping condition
        # TODO: make this more general
        def StoppingCondition(datum):
            return True
        
        # Define the injector object
        self.injector = LI.injection.InjectorBase(self.events_to_inject,
                                                  self.earth_model, 
                                                  self.primary_injection_process, 
                                                  self.secondary_injection_processes,
                                                  self.random)
        
        self.injector.SetStoppingCondition(StoppingCondition)

        # Define the weighter object
        self.weighter = LI.injection.LeptonTreeWeighter([self.injector],
                                                        self.earth_model, 
                                                        self.primary_physical_process, 
                                                        self.secondary_physical_processes)
        
    def GenerateEvents(self,N=None):
        if N is None:
            N = self.events_to_inject
        count = 0
        while (self.injector.InjectedEvents() < self.events_to_inject) \
               and (count < N):
            print('Injecting Event %d/%d  '%(count,N),end='\r')
            tree = self.injector.GenerateEvent()
            self.events.append(tree)
            count += 1
        return self.events

    def SaveEvents(self,filename):
        fout = h5py.File(filename,'w')
        fout.attrs['num_events'] = len(self.events)
        for ie,event in enumerate(self.events):
            print('Saving Event %d/%d  '%(ie,len(self.events)),end='\r')
            event_group = fout.require_group("event%d"%ie)
            event_group.attrs['event_weight'] = self.weighter.EventWeight(event)
            event_group.attrs['num_interactions'] = len(event.tree)
            for id,datum in enumerate(event.tree):
                interaction_group = event_group.require_group("interaction%d"%id)
                
                # Add metadata on interaction signature
                interaction_group.attrs['primary_type'] = str(datum.record.signature.primary_type)
                interaction_group.attrs['target_type'] = str(datum.record.signature.target_type)
                for isec,secondary in enumerate(datum.record.signature.secondary_types):
                    interaction_group.attrs['secondary_type%d'%isec] = str(secondary)

                # Save vertex as dataset
                interaction_group.create_dataset('vertex',data=np.array(datum.record.interaction_vertex,dtype=float))
                
                # Save each four-momenta as a dataset
                interaction_group.create_dataset('primary_momentum',data=np.array(datum.record.primary_momentum,dtype=float))
                interaction_group.create_dataset('target_momentum',data=np.array(datum.record.primary_momentum,dtype=float))
                for isec_momenta,sec_momenta in enumerate(datum.record.secondary_momenta):
                    interaction_group.create_dataset('secondary_momentum%d'%isec_momenta,data=np.array(sec_momenta,dtype=float))

        fout.close()

