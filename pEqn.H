{
    //-Calculate descretization coefficients based on the definition of Ueqn
    volScalarField rAU("rAU",1.0/UEqn.A());
    surfaceScalarField rAUf("rAUf",fvc::interpolate(rAU));

    surfaceScalarField rhof = twoPhaseProperties.rhoMixF(scalar(0.5)*(alpha1 + alpha1.oldTime()));

    //-Solve for the velocity based on the velocity terms described in UEqn.
    volVectorField HbyA("HbyA",U);
    HbyA = rAU*UEqn.H();

    surfaceScalarField phiHbyA
    (
        "phiHbyA",
        (fvc::interpolate(HbyA)& mesh.Sf())
      + fvc::ddtPhiCorr(rAU, rho, U, phi)
    );

    adjustPhi(phiHbyA, U, p_rgh);
    phi = phiHbyA;

    surfaceScalarField phig
    (
        K_SandG*rAUf
    );

    phiHbyA += phig;

    while (pimple.correctNonOrthogonal())
    {
        fvScalarMatrix p_rghEqn
        (
            fvm::laplacian(rAUf, p_rgh) == fvc::div(phi) 
        );
        
        p_rghEqn.setReference(pRefCell, getRefCellValue(p_rgh, pRefCell));

        p_rghEqn.solve(mesh.solver(p_rgh.select(pimple.finalInnerIter())));

        if (pimple.finalNonOrthogonalIter())
        {
            phi = phiHbyA - p_rghEqn.flux();

            U = HbyA + rAU*fvc::reconstruct((phig - p_rghEqn.flux())/rAUf);
            U.correctBoundaryConditions();
            fvOptions.correct(U);
        }
    }

    #include "continuityErrs.H"

    p == p_rgh + rho*gh;

    if (p_rgh.needReference())
    {
        p += dimensionedScalar
        (
            "p",
            p.dimensions(),
            pRefValue - getRefCellValue(p, pRefCell)
        );
        p_rgh = p - rho*gh;
    }
}