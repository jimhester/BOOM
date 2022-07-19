import BayesBoom.boom as boom
import pandas as pd
import numpy as np
import BayesBoom.R as R


class BetaBinomialMixture:
    """
    A finite mixture of BetaBinomial distributions.

    Example use:

    # Create an empty model object.
    model = BetaBinomialMixture()

    # To fit a 3 component mixture, add three components.
    model.add_component(R.BetaPrior(1.0, 1.0), R.UniformPrior(0.1, 1000.0))
    model.add_component(R.BetaPrior(1.0, 1.0), R.UniformPrior(0.1, 1000.0))
    model.add_component(R.BetaPrior(1.0, 1.0), R.UniformPrior(0.1, 1000.0))

    # Add the data.
    model.add_data(my_three_column_matrix)

    # Fit the model by running MCMC for 'niter' iterations.
    model.mcmc(niter=1000)

    """

    def __init__(self):
        self._components = list()
        self._mixing_distribution_prior_counts = list()
        self._boom_model = None

    def add_component(self,
                      mean_prior: R.BetaPrior,
                      sample_size_prior: R.DoubleModel,
                      prior_count: float = 1.0):
        """
        Add a beta-binomial mixture component to the model.  The beta-binomial
        distribution is parameterized by two parameters: a and b, which can be
        thought of as counts of successes (a) and failures (b) in a binomial
        inference problem.  The mean of the distribution is a/(a+b), and the
        variance is controlled by the "sample size" a+b.

        Args:
          mean_prior: A prior distribution on the mean (a/a+b) of the component.
          sample_size_prior: A prior distribution on the sample size (a+b) of
            the component.
          prior_count: The mixing distribution has a dirichlet prior, which is
            parameterized by a vector of positive real numbers interpretable as
            prior counts.  This argument is the prior count for the frequency
            of this component in the mixture.
        """
        self._mixing_distribution_prior_counts.append(prior_count)
        self._mixing_weights = None
        component = {
            "mean_prior": mean_prior,
            "sample_size_prior": sample_size_prior
        }
        self._components.append(component)

    def add_data(self, data):
        """
        data: A three-column matrix of numbers giving the (a) number of trials,
          (b) the number of successes, and (c) the number of cases with that
          many trials and successes.

        Example:
        10, 4, 28   # There were 28 cases of 10 trials showing 4 successes.
         5, 0, 2    # There were 2 cases of 5 trials showing no successes.
        """
        if self._boom_model is None:
            self._boom_model = self._build_boom_model()
        if isinstance(data, pd.DataFrame):
            data = data.values
        data = data.astype(int)
        self._boom_model.add_data(data[:, 0], data[:, 1], data[:, 2])

    def mcmc(self, niter, ping=None, seed=None):
        """
        Run the MCMC algorithm for 'niter' iterations.

        Args:
          niter:  The desired number of MCMC iterations.
          ping:  The frequency with which to print status updates.
          seed:  The random seed to use for the C++ random number generator.

        Returns:
          None

        Effects:
          MCMC draws are stored in the object.
        """
        if self._boom_model is None:
            self._boom_model = self._build_boom_model()
            import warnings
            warnings.warn("Running MCMC on a model with no data assigned.")

        self._create_storage(niter)
        if seed is not None:
            # I don't have a lot of confidence that this works.
            boom.GlobalRng.rng.seed(int(seed))
        for i in range(niter):
            R.report_progress(i, ping)
            self._boom_model.sample_posterior()
            self._record_draw(i)

    @property
    def number_of_mixture_components(self):
        """
        The number of mixture components contained in the object.
        """
        return len(self._mixing_distribution_prior_counts)

    @property
    def niter(self):
        """
        The number of MCMC iterations that have been run.
        """
        if self._mixing_weights is None:
            return 0
        return self._mixing_weights.shape[0]

    @property
    def a(self):
        """
        MCMC draws of the 'success count' parameters.  This is a matrix with
        'niter' rows and 'number_of_mixture_components' columns, with each
        column representing the 'a' parameter of one mixture component.
        """
        ans = np.empty((self.niter, self.number_of_mixture_components))
        for i in range(self.number_of_mixture_components):
            ans[:, i] = self._components["draws"][:, 0]
        return ans

    @property
    def b(self):
        """
        MCMC draws of the 'failure count' parameters.  This is a matrix with
        'niter' rows and 'number_of_mixture_components' columns, with each
        column representing the 'b' parameter of one mixture component.
        """
        ans = np.empty((self.niter, self.number_of_mixture_components))
        for i in range(self.number_of_mixture_components):
            ans[:, i] = self._components["draws"][:, 1]
        return ans

    @property
    def means(self):
        """
        MCMC draws of the mean parameter for each mixture component.
        """
        a = self.a
        b = self.b
        return a / (a + b)

    @property
    def sample_sizes(self):
        """
        MCMC draws of the sample size parameter for each mixture component.
        """
        return self.a + self.b

    @property
    def mixing_weights(self):
        """
        MCMC draws of the mixing weights.
        """
        return self._mixing_weights

    def _create_storage(self, niter: int):
        """
        Create storage for 'niter' MCMC draws.
        """
        for component in self._components:
            component["draws"] = np.empty((niter, 2))
        self._mixing_weights = np.empty(
            (niter, self.number_of_mixture_components))

    def _record_draw(self, i):
        for c, component in enumerate(self._components):
            component["draws"][i, :] = [
                self._boom_model.mixture_component(c).a,
                self._boom_model.mixture_component(c).b
            ]
        self._mixing_weights[i, :] = (
            self._boom_model.mixing_distribution.probs.to_numpy()
        )

    def _build_boom_model(self):
        """
        Build a boom_model from
        """
        prior_counts = np.array(self._mixing_distribution_prior_counts)
        mixing_distribution = boom.MultinomialModel(prior_counts.shape[0])
        dirichlet_prior = boom.DirichletModel(R.to_boom_vector(prior_counts))
        mixing_distribution_sampler = boom.MultinomialDirichletSampler(
            mixing_distribution, dirichlet_prior)
        mixing_distribution.set_method(mixing_distribution_sampler)

        boom_components = list()
        for component in self._components:
            mean_prior = component["mean_prior"].boom()
            sample_size_prior = component["sample_size_prior"].boom()
            component_model = boom.BetaBinomialModel(1.0, 1.0)
            component_sampler = boom.BetaBinomialPosteriorSampler(
                component_model, mean_prior, sample_size_prior)
            component_model.set_method(component_sampler)
            boom_components.append(component_model)

        boom_model = boom.BetaBinomialMixtureModel(
            boom_components, mixing_distribution)
        sampler = boom.BetaBinomialMixturePosteriorSampler(boom_model)
        boom_model.set_method(sampler)
        return boom_model
